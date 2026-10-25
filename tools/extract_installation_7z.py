#!/usr/bin/env python3
"""Extract a user-owned installation with system libarchive, outside the repository.
Reject links, traversal, overwrites and unexpectedly large members. No assets are
included in this tool. libarchive must support the supplied archive format.
"""
import argparse,ctypes as c,ctypes.util,json,os
from pathlib import Path,PurePosixPath
p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('destination',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];dest=a.destination.resolve()
if dest==repo or repo in dest.parents:raise ValueError('Keep installation assets outside the repository')
lib=c.CDLL(ctypes.util.find_library('archive') or 'libarchive.so.13')
def bind(name,result,args):
 f=getattr(lib,name);f.restype=result;f.argtypes=args;return f
ptr=c.c_void_p
new=bind('archive_read_new',ptr,[]);filters=bind('archive_read_support_filter_all',c.c_int,[ptr]);formats=bind('archive_read_support_format_all',c.c_int,[ptr]);open_file=bind('archive_read_open_filename',c.c_int,[ptr,c.c_char_p,c.c_size_t]);next_header=bind('archive_read_next_header',c.c_int,[ptr,c.POINTER(ptr)]);read=bind('archive_read_data',c.c_ssize_t,[ptr,ptr,c.c_size_t]);free=bind('archive_read_free',c.c_int,[ptr]);error=bind('archive_error_string',c.c_char_p,[ptr]);name=bind('archive_entry_pathname',c.c_char_p,[ptr]);kind=bind('archive_entry_filetype',c.c_uint,[ptr]);size=bind('archive_entry_size',c.c_int64,[ptr]);hardlink=bind('archive_entry_hardlink',c.c_char_p,[ptr])
reader=new();count=total=0;dest.mkdir(parents=True,exist_ok=True);buffer=c.create_string_buffer(1024*1024)
def ok(status):
 if status<0:raise RuntimeError((error(reader) or b'libarchive failure').decode(errors='replace'))
try:
 ok(filters(reader));ok(formats(reader));ok(open_file(reader,os.fsencode(a.archive),1024*1024));entry=ptr()
 while True:
  status=next_header(reader,c.byref(entry))
  if status==1:break
  ok(status);raw=name(entry).decode('utf-8').replace('\\','/');relative=PurePosixPath(raw)
  if relative.is_absolute() or '..' in relative.parts or ':' in raw:raise ValueError('Unsafe archive path')
  target=dest.joinpath(*relative.parts)
  if dest not in target.resolve().parents and target.resolve()!=dest:raise ValueError('Path escapes extraction root')
  filetype=kind(entry)
  if hardlink(entry):raise ValueError('Archive hard links unsupported')
  if filetype==0o040000:target.mkdir(parents=True,exist_ok=True);continue
  if filetype!=0o100000:raise ValueError('Archive special files unsupported')
  expected=size(entry)
  if not 0<=expected<=2*1024**3:raise ValueError('Archive member size outside bounds')
  if target.exists():raise FileExistsError(target)
  target.parent.mkdir(parents=True,exist_ok=True);partial=target.with_name(target.name+'.partial');written=0
  created=False
  try:
   with partial.open('xb') as output:
    created=True
    while True:
     n=read(reader,buffer,len(buffer));ok(n)
     if not n:break
     written+=n
     if written>expected:raise ValueError('Archive member exceeds declared size')
     output.write(buffer.raw[:n])
   if written!=expected:raise ValueError('Archive member size mismatch')
   os.link(partial,target)
   partial.unlink()
  except BaseException:
   if created:partial.unlink(missing_ok=True)
   raise
  count+=1;total+=written
  if count>100000:raise ValueError('Archive file count limit exceeded')
finally:
 free(reader)
print(json.dumps({'files':count,'bytes':total,'destination':str(dest)}))
