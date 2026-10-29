#!/usr/bin/env python3
"""Original event queue with native synchronization and real filtered flushing."""
import argparse,hashlib,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];root=a.upstream.resolve();out=a.output.resolve()
if out==root or root in out.parents or out in root.parents or out==repo or repo in out.parents or out in repo.parents:raise ValueError('External staging required')
manifest=json.loads((repo/'cmake/engine-event-source-hashes.json').read_text());texts={}
for name,digest in manifest.items():
 data=(root/name).read_bytes()
 if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Unreviewed event source '+name)
 texts[Path(name).name]=data.decode()
queue=texts['queue.h'].replace('#include "system.h"','#include "system_sync.h"')
index=queue.index('template <class DATA, int SIZE> class SafeQueue')
ahead,tail=queue[:index],queue[index:]
tail=tail.replace('return (head == tail ? TRUE : FALSE);','return (!sem_items.Test());').replace('return (FALSE);\n    }\n\n    //\n    // Wait:', 'return (!sem_space.Test());\n    }\n\n    //\n    // Wait:')
texts['queue.h']=ahead+tail
text=texts['event.cpp'];text='#include <mutex>\n#include <vector>\n'+text
text=text.replace('namespace EventSys\n{','namespace EventSys\n{\n  static std::recursive_mutex queueAccess;')
for signature in ['Bool Get(Event &ev)','Bool Put(Event &ev)']:
 text=text.replace(signature+'\n  {',signature+'\n  {\n    std::lock_guard<std::recursive_mutex> guard(queueAccess);')
text=text.replace('return (queue.AddPre(0));','queueAccess.lock();\n    Event *result=queue.AddPre(0);\n    if (!result) queueAccess.unlock();\n    return result;')
text=text.replace('void PutUnlock()\n  {\n    ASSERT(sysInit);\n    queue.AddPost();','void PutUnlock()\n  {\n    ASSERT(sysInit);\n    queue.AddPost();\n    queueAccess.unlock();')
old='''    // FIXMEE - not implemented yet
    type;

    queue.Flush();'''
if old not in text:raise ValueError('Unexpected event flush source')
text=text.replace(old,'''    std::lock_guard<std::recursive_mutex> guard(queueAccess);
    std::vector<Event> keep;
    Event event;
    while (Get(event)) if (event.type != type) keep.push_back(event);
    for (auto &saved : keep) { if (!Put(saved)) ERR_FATAL(("Event flush overflow")); }''')
text=text.replace('    handlers.DisposeAll();','    Event discarded;\n    while (Get(discarded)) {}\n    handlers.DisposeAll();')
texts['event.cpp']=text
out.mkdir(parents=True,exist_ok=True)
for name,text in texts.items():
 dest=out/name
 if not dest.exists() or dest.read_text()!=text:dest.write_text(text)
