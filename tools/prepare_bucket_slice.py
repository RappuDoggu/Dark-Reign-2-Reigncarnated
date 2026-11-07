#!/usr/bin/env python3
"""Stage the original opaque bucket manager externally with reviewed native edits."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1]
    output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Overlay must be outside repository')
    pins=json.loads((repo/'cmake/bucket-source-hashes.json').read_text())
    texts={}
    for name,digest in pins.items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed bucket source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    edits=[]
    def replace(text,old,new,label):
        if text.count(old)!=1:raise ValueError('Unexpected source shape: '+label)
        edits.append(label);return text.replace(old,new)
    output.mkdir(parents=True,exist_ok=True)
    primitive=texts['graphics/Primitive.h']
    primitive=replace(primitive,'#include "material.h"','#include "reign/bucket_prelude.hpp"','primitive material boundary')
    primitive=replace(primitive,'#define DOVERTEXNON32','#undef DOVERTEXNON32\n//','TL-only vertex layout in this dependency slice')
    (output/'primitive.h').write_text(primitive)
    body=texts['graphics/Primitive.cpp']
    body=replace(body,'memset( textureStages, NULL,','memset( textureStages, 0,','standard zero fill argument')
    (output/'primitive.cpp').write_text(body)
    desc=texts['graphics/BucketDesc.h']
    desc=desc[:desc.index('// description of how to lock memory for a group of polys')]+ '\n#endif\n'
    desc=desc.replace('#include "vertex.h"','')
    (output/'bucketdesc.h').write_text(desc)
    header=texts['graphics/bucket.h'].replace('#include "vertex.h"','#include "nlist.h"').replace('"Bucketdesc.h"','"bucketdesc.h"')
    (output/'bucket.h').write_text('#include "reign/bucket_prelude.hpp"\n'+header)
    descsrc=texts['graphics/BucketDesc.cpp'].replace('#include "vid.h"','').replace('"Bucket.h"','"bucket.h"')
    if descsrc.count('.vv.z')!=6:raise ValueError('Unexpected sort Z accessors')
    descsrc=descsrc.replace('.vv.z','.z')
    descsrc=replace(descsrc,'  offset = vCount;',
        '  if (std::uint64_t(vCount) + _vCount > 65536u)throw std::runtime_error("Bucket vertex index overflow");\n'
        '  for (U32 i=0;i<_iCount;++i) {\n'
        '    U32 value=iMemCur[i];\n'
        '    if (doOffset) {if(value>=_vCount || value+vCount>65535u)throw std::runtime_error("Invalid relative bucket index");}\n'
        '    else if(value>=vCount+_vCount)throw std::runtime_error("Invalid absolute bucket index");\n'
        '  }\n  offset = vCount;','bounded index rebasing before buffer mutation')
    descsrc=replace(descsrc,'  curBucket = this;',
        '  ASSERT(iCount % 3 == 0);\n'
        '  for(U32 i=0;i<iCount;++i) {ASSERT(iMem[i]<vCount);ASSERT(std::isfinite(((VertexTL *)vMem)[iMem[i]].z));}\n'
        '  curBucket = this;','validate sort inputs before comparator dereference')
    (output/'bucketdesc.cpp').write_text(descsrc)
    body=texts['graphics/bucket.cpp']
    body=body[body.index('PrimitiveDesc\t\t  BucketMan::primitive;'):]
    start=body.index('void Vid::InitBuckets()');end=body.index('void BucketMan::ClearData()',start)
    body=body[:start]+body[end:]
    body=replace(body,'\tU32 bucketCount = bucketList.GetCount();','\treign::bucket::validate_init(_count,_size,_ratio);\n\tU32 bucketCount = bucketList.GetCount();','bounded bucket pool allocation')
    body=replace(body,'  U32 iSize = _iCount << 1;',
        '  U32 iSize = ((_iCount << 1) + 3u) & ~3u;','aligned index region and following batch')
    body=replace(body,'U32 bsize = curSize >= sizeofBucket + 10 ? sizeofBucket : curSize;',
        'U32 bsize = (curSize >= sizeofBucket + 10 ? sizeofBucket : curSize) & ~3u;','aligned bucket memory slice')
    body=replace(body,'U32 vs = (U32) Utils::FtoLDown(((F32) extra) * memRatio);',
        'U32 vs = ((U32) Utils::FtoLDown(((F32) extra) * memRatio)) & ~3u;','aligned vertex/index split')
    body=replace(body,'bucket.vCountLeft = bucket.vCountMax = vSize / sizeof( VertexTL);',
        'bucket.vCountLeft = bucket.vCountMax = std::min<U32>(65536u, vSize / sizeof(VertexTL));','16-bit index vertex capacity')
    # Indexed/nonindexed entry points share a padding statement; split by function.
    if body.count('  vCount += 3;')==2:
        first=body.index('  vCount += 3;')
        body=body[:first]+body[first:].replace('  vCount += 3;',
            '  reign::bucket::validate_request(vCount,iCount);\n  ASSERT(primitive.vertex_type == FVF_TLVERTEX);\n  vCount += 3;',1)
        second=body.index('  vCount += 3;',body.index('Bucket * BucketMan::LockPrimitiveMem'))
        body=body[:second]+body[second:].replace('  vCount += 3;',
            '  reign::bucket::validate_request(vCount,0);\n  ASSERT(primitive.vertex_type == FVF_TLVERTEX);\n  vCount += 3;',1)
        edits.append('checked indexed/nonindexed request sizes and FVF')
    else:raise ValueError('Unexpected lock padding statements')
    body=replace(body,'  TextureStage & stage0 = bucket.textureStages[0];',
        '  if(bucket.texture_count>1)throw std::runtime_error("Multitexture bucket not implemented");\n'
        '  TextureStage & stage0 = bucket.textureStages[0];','reject active unsupported bucket texture stages')
    if body.count('this == &Vid::bucket ? "" : "Tran"')!=2:raise ValueError('Unexpected bucket diagnostic globals')
    body=body.replace('this == &Vid::bucket ? "" : "Tran"','""')
    edits.append('drop unavailable global only from diagnostic prefix')
    (output/'bucket.cpp').write_text('// Selected original implementation: Copyright 1997-1999 Pandemic Studios, Dark Reign II.\n#include "bucket.h"\n'+body)
    th=texts['graphics/TranBucket.h']
    for name in ['SetMaxZ','SetMinZ','SetMaxBucketCount']:
        th=replace(th,'void TranBucketMan::'+name,'void '+name,'standard in-class '+name+' declaration')
    (output/'tranbucket.h').write_text(th)
    tb=texts['graphics/TranBucket.cpp'].replace('#include "vid_public.h"','').replace('"TranBucket.h"','"tranbucket.h"')
    tb=replace(tb,'Bucket * list[444];','std::vector<Bucket *> list(bucketList.GetCount());','sized translucent sorting scratch space')
    tb=replace(tb,'ASSERT( count <= 444);','ASSERT( U32(count) <= list.size());','sorting scratch count bound')
    (output/'tranbucket.cpp').write_text(tb)
    (output/'nlist.h').write_text(texts['system/nlist.h'])
    (output/'portability-manifest.json').write_text(json.dumps(dict(edits=edits),indent=2)+'\n')
    print('Staged original PrimitiveDesc/Bucket/BucketMan externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)
