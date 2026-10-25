#!/usr/bin/env python3
"""Stage original IFace event/focus/modal methods outside the repository."""
import argparse, hashlib, json, re
from pathlib import Path

def prepare(upstream, output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('External source staging required')
    texts={}
    for name,digest in json.loads((repo/'cmake/iface-event-hashes.json').read_text()).items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed IFace source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    source=texts['interface/iface.cpp']
    names=['SetFlag','GetFlag','SendEvent','PostEvent','SendNotify','SendNotifyModal','SendNotifyFocus',
           'PostNotify','PostNotifyModal','PostNotifyFocus','EventID','SetTipControl',
           'SetMouseCapture','ReleaseMouseCapture','GetCapture','GetMouseOver','SetFocus','ReleaseFocus',
           'GetFocus','SetModal','UnsetModal','GetModal','DeactivateModals','IFaceEventHandler',
           'RegisterControlClass','UnregisterControlClass','FindControlClass','DefineControlType']
    bodies=[]
    for name in names:
        match=re.search(r'^  (?:Bool|U32|U16|void|IControl \*|ICClass \*)\s*'+name+r'\([^\n]*\)\s*\n  \{',source,re.M)
        if not match:raise ValueError('Missing function '+name)
        a=match.start();b=source.index('{',a)+1;depth=1
        while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
        body=source[a:b]
        if name=='PostEvent':
            # Allocate/validate Reapers before reserving the native mutex queue.
            # Queue-full rejection and sender/recipient ownership stay explicit.
            body=body.replace('    Event *e = EventSys::PutLock();',
                '    if (EventSys::QueueFull()) return FALSE;\n'
                '    std::unique_ptr<IControlPtr> ownedTo(new IControlPtr(ctrl));\n'
                '    std::unique_ptr<IControlPtr> ownedFrom(from ? new IControlPtr(from) : nullptr);\n'
                '    Event *e = EventSys::PutLock();')
            begin=body.index('      // Create reapers');end=body.index('      // Build up the event structure',begin)
            body=body[:begin]+'      IControlPtr *ptrTo=ownedTo.get(), *ptrFrom=ownedFrom.get();\n\n'+body[end:]
            body=body.replace('      EventSys::PutUnlock();','      EventSys::PutUnlock();\n      ownedTo.release(); ownedFrom.release();')
        if name in ['UnregisterControlClass','DefineControlType']:
            body=body.replace('ASSERT(sysInit);','ASSERT(reign::iface_event_detail::state().registryReady);')
        if name=='DefineControlType':
            # Fatal errors are exceptions on the port. Reject before mutation:
            # original BinTree::Add inserts a duplicate before reporting it.
            body=body.replace('    // It was so create a new Control class',
                '    if (ctrlTypes.Find(Crc::CalcStr(newClass)))\n'
                '    { ERR_FATAL(("Control class [%s] already defined", newClass)); }\n\n'
                '    // It was so create a new Control class')
        fields=['ctrlTypes','eventId','systemFlags','modalStack','modal','focus','mouseCapture','mouseOver','tipCtrl']
        # Rewrite identifiers only, preserving original diagnostics and comments.
        token=r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\b(?:'+ '|'.join(fields)+r')\b'
        body=re.sub(token,lambda m:'reign::iface_event_detail::state().'+m[0] if m[0] in fields else m[0],body)
        bodies.append(body)
    output.mkdir(parents=True,exist_ok=True)
    old='U32 PostNotify(IControl *from, U32 event, U32 p1 = 0, U32 p2 = 0, U32 p3 = 0);'
    if texts['interface/iface.h'].count(old)!=1:raise ValueError('Unexpected PostNotify declaration')
    (output/'iface.h').write_text(texts['interface/iface.h'].replace(old,old.replace('IControl *from','IControl *ctrl, IControl *from')))
    (output/'iface_stack.hpp').write_text(texts['system/stack.h'])
    (output/'iface_events.hpp').write_text('''#pragma once
#include "control_types.hpp"
struct Pix;
#include "iface.h"
#include "reign/frontend_prelude.hpp"
#include "iface_stack.hpp"
#include <memory>
namespace reign::iface_event_detail {
struct State {
 U16 eventId=0; U32 systemFlags=0;
 // Readiness of this registry only; does not claim full IFace initialization.
 Bool registryReady=TRUE; BinTree<ICClass> ctrlTypes;
 Stack<IControlPtr> modalStack;
 IControlPtr modal,focus,mouseCapture,mouseOver,tipCtrl;
 ~State(){modalStack.DisposeAll();ctrlTypes.DisposeAll();}
};
State &state();
struct Binding {State *previous;explicit Binding(State &);~Binding();};
}
namespace IFace { Bool IFaceEventHandler(Event &); }
''')
    (output/'iface_events.cpp').write_text('#include "iface_events.hpp"\n#define PROCESS(c,e,s) (c)->HandleEvent(e)\nnamespace IFace {\n'+'\n\n'.join(bodies)+'\n}\n#undef PROCESS\n')
    print('Staged 28 original IFace event/state methods; native PostEvent ownership guard enabled')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();prepare(a.upstream,a.output)
