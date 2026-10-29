#!/usr/bin/env python3
"""Stage complete original control/root/button/window translation units externally.
Produces compilation targets, not a linkable or runnable frontend. Movie handling,
font/interface services and several utility implementations remain link requirements.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path


def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('Original source must be staged outside the repository')
    texts = {}
    hashes = json.loads((repo/'cmake/frontend-source-hashes.json').read_text())
    for name, digest in hashes.items():
        raw = (upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('Unreviewed frontend source: '+name)
        texts[name] = raw.decode().replace('\r\n', '\n')
    output.mkdir(parents=True, exist_ok=True)
    def replace_once(s, old, new):
        if s.count(old) != 1:
            raise ValueError('Expected one occurrence: '+old)
        return s.replace(old, new)
    reapers = texts['system/reaperlist.h'].replace('#include "dtrack.h"','')
    reapers = reapers.replace('public:', 'public:\n  using Iterator = typename NList<NODE>::Iterator;',1)
    reapers = reapers.replace('NList<NODE>::Node *', 'typename NList<NODE>::Node *')
    for method in ['SetNodeMember','GetHeadNode','GetTailNode','GetHead','Unlink','Dispose','GetCount']:
        reapers = re.sub(r'(?<![\w:.>])'+method+r'\(', 'this->'+method+'(', reapers)
    (output/'native_reaperlist.hpp').write_text(reapers)
    for name in ['icontrol', 'icroot', 'icbutton', 'icwindow', 'ifvar', 'icsystembutton', 'messagebox_event', 'iface_messagebox', 'icstatic', 'iclistbox', 'iclistslider', 'icslider', 'icmenu', 'icedit', 'ictipwindow', 'ictabgroup', 'console_edit', 'console_viewer', 'colorbutton', 'coloreditor', 'icdroplist', 'icmonoview', 'ickeyview']:
        s = texts['interface/'+name+'.cpp']
        s = s.replace('#include "input.h"', '#include "control_input.hpp"')
        s = s.replace('#include "stdload.h"', '#include "control_stdload.hpp"')
        # Explicit dependencies in frontend_prelude replace legacy precompiled headers.
        for header in ['vid.h', 'debug_memory.h', 'perfstats.h']:
            s = s.replace('#include "'+header+'"', '')
        if name in ['iclistbox','iclistslider','icslider']:
            s = '#include "reign/listbox_support.hpp"\n'+s
        if name == 'ickeyview':
            s = '#include "iclistbox.h"\n#include "key_names_slice.hpp"\n'+s.replace('KeyBind::FindKeyByScan', 'reign::key_name_detail::FindKeyByScan')
        if name == 'icmonoview':
            s = '#define MONO_DISABLED\n#include "reign/listbox_support.hpp"\n'+s.replace('#include "mono.h"','').replace('#include "font.h"','')
            s = re.sub(r'\b(\d+)L\b', r'S32(\1)', s)
            for color in ['colorBg','colorFg','colorBright']:s=s.replace(color+'.Set(',color+'.SetInline(')
        if name == 'icdroplist':
            (output/'icdroplist.h').write_text('#include "iclistbox.h"\n#include "ifvar.h"\n'+texts['interface/icdroplist.h'])
            s = s.replace('controlStyle |= TRANSPARENT;', 'controlStyle |= STYLE_TRANSPARENT;').replace('delete selectedVarName;', 'delete[] selectedVarName;')
            s = s.replace('#include "babel.h"', '#include "babel_slice.hpp"')
            s = s.replace('[Drop%08X:%s]', '[Drop%p:%s]').replace('ContainerCtrlName, this, Name()', 'ContainerCtrlName, static_cast<void *>(this), Name()')
        if name == 'colorbutton':
            s = s.replace('SetTabStop(FALSE)', 'SetTabStop(nullptr)')
        if name == 'coloreditor':
            s = re.sub(r'\b(\d+)l\b', r'S32(\1)', s)
            s = s.replace('bar.Set(', 'bar.SetInline(')
            s = s.replace('color1.Set(color2.R(), color2.G(), color2.B(), 0.0f)', 'color1.SetInline(S32(color2.R()), S32(color2.G()), S32(color2.B()), S32(0))')
        if name == 'console_viewer':
            header=texts['util/stdload.h'];start=header.index('  template <U32 SIZE> void TypeStrCrcList(FScope *fScope, List<StrCrc<SIZE> > &strings)');end=header.index('{',start)+1;depth=1;cursor=end
            while depth:
                depth+=(header[cursor]=='{')-(header[cursor]=='}');cursor+=1
            (output/'console_viewer_support.hpp').write_text('#pragma once\n#include "console_slice.hpp"\n#include "control_stdload.hpp"\nnamespace StdLoad {\n'+header[start:cursor]+'\n}\n')
            s='#include "console_viewer_support.hpp"\n'+s
            s=s.replace('#include "console.h"','#include "console_slice.hpp"')
        if name == 'console_edit':
            s='#include "reign/console_edit_support.hpp"\n'+s
            s=s.replace('#include "console.h"','#include "console_slice.hpp"').replace('#include "console_edit.h"','#include "native_console_edit.hpp"')
            (output/'native_console_edit.hpp').write_text(texts['interface/console_edit.h'].replace('#include "console.h"','#include "reign/console_edit_support.hpp"'))
        if name == 'icedit':
            s='#include "reign/edit_support.hpp"\n'+s
            s=s.replace('#include "fontsys.h"','').replace('0L','S32(0)').replace('(long)0','S32(0)').replace('1L','S32(1)')
            for sig in ['void ICEdit::ClipboardCopy(Bool cut)','void ICEdit::ClipboardPaste()']:
                start=s.index(sig);end=s.index('{',start)+1;depth=1;cursor=end
                while depth:
                    depth+=(s[cursor]=='{')-(s[cursor]=='}');cursor+=1
                s=s[:end]+'\n  throw std::runtime_error("Native clipboard service not bound");\n}'+s[cursor:]
        if name == 'iclistbox':
            start=s.index('U32 ICListBox::FillFromPath(');end=s.index('{',start)+1;depth=1;cursor=end
            while depth:
                depth+=(s[cursor]=='{')-(s[cursor]=='}');cursor+=1
            s=s[:end]+'\n  throw std::runtime_error("Listbox directory enumeration not bound");\n}'+s[cursor:]
        if name == 'iclistslider':
            s=s.replace(', 0L)', ', S32(0))')
        if name == 'iface_messagebox':
            s = replace_once(s, 'for (U32 b = 0; events[b] && (b < 3); b++)',
                             'for (U32 b = 0; (b < 3) && events[b]; b++)')
            s = replace_once(s, '        lineBuf[lineCount++] = s;',
                             '        if (lineCount >= MAX_LINES) throw std::length_error("Too many message box lines");\n        lineBuf[lineCount++] = s;')
            # Both escape branches increment s. Do not advance beyond final NUL.
            if s.count('          s++;') != 2:
                raise ValueError('Message line scanner changed')
            s = s.replace('          s++;', "          if (!s[1]) break;\n          s++;")
            s = replace_once(s, '    ASSERT(btn1);',
                '    ASSERT(btn1);\n    if (btn3 && !btn2) throw std::invalid_argument("Noncontiguous message buttons");\n    if (Utils::Strlen(string) >= 1024) throw std::length_error("Message text too long");')
        if name == 'icstatic':
            s = '#include "babel_slice.hpp"\n#include <cmath>\n'+s
            s = replace_once(s, '        Utils::Sprintf(buf, 128, (const CH*)L"%02u:%02u", (U32)(v / 60.0F), (U32)fmod(v, 60.0F));',
                '        if (!std::isfinite(v) || v < 0 || double(v)/60 > 2147483647.0) throw std::range_error("Static timer out of range");\n        Utils::Strmcpy(buf, Babel::Translate("{1:02u}:{2:02u}", 2, int(v/60), int(std::fmod(v,60))), 128);')
        if name == 'ifvar':
            macro = re.search(r'^#define MULTILANGUAGE_MAXDATA .*$', texts['util/multilanguage.h'], re.M).group(0)
            s = replace_once(s, '#include "multilanguage.h"', macro)
        if name == 'icsystembutton':
            s = 'class ICListBox; using ICListBoxPtr=Reaper<ICListBox>;\n#include "icslider.h"\n'+s
        if name == 'icontrol':
            if s.count('Color(0L, 0L, 0L, 128L)') != 2:
                raise ValueError('Unexpected color construction sites')
            s = s.replace('Color(0L, 0L, 0L, 128L)', 'Color(S32(0), S32(0), S32(0), S32(128))')
            s = replace_once(s, 'alignTo->parent ? alignTo->parent : alignTo;',
                             'alignTo->parent ? alignTo->parent : alignTo.GetPointer();')
            s = replace_once(s, "char *dot = Utils::Strrchr(stripped, '.');",
                             "const char *dot = Utils::Strrchr(stripped, '.');")
            start = s.index('  if (texture && texture->texture && texture->texture->GetBink()')
            end = s.index('\n  // Notify control that is was deactivated', start)
            # This adapter is intentionally UNDEFINED, never a successful no-op.
            s = s[:start]+'  if (texture && texture->texture) reign::deactivate_control_movie(texture->texture);\n'+s[end:]
        s = s.replace('IFace::data', 'IFace::FrontendData()')
        (output/(name+'.cpp')).write_text(s)
    math = texts['util/utiltypes.cpp']
    start = math.index('F32 Value(')
    (output/'color_math.cpp').write_text('#include "control_types.hpp"\n'+math[start:].replace('color.Set(', 'color.SetInline('))
    # Preserve the original variable-name/dereference path; IControl supplies
    # parent-relative lookup. This remains unexecuted until live controls link.
    source = texts['interface/iface_util.cpp']
    a = source.index('  Bool ExpandDerefVar(')
    b = source.index('{', a) + 1
    depth = 1
    while depth:
        depth += (source[b] == '{') - (source[b] == '}')
        b += 1
    helpers = [source[a:b]]
    for signature in ['  const CH * BreakText(', '  void RenderSheets(', '  void UpdateSheets(', '  void FScopeToSheetInfo(']:
        a = source.index(signature); b = source.index('{',a)+1; depth = 1
        while depth:
            depth += (source[b]=='{')-(source[b]=='}'); b += 1
        body = source[a:b]
        if 'BreakText(' in signature:
            body=body.replace('{','{\n  if(!src||!font) throw std::invalid_argument("Text wrapping requires text and font");',1)
        if 'FScopeToSheetInfo(' in signature:
            body = replace_once(body, '    sheets.x = U8(StdLoad::TypeU32(fScope, 2));\n    sheets.y = U8(StdLoad::TypeU32(fScope, 2));',
                '    U32 sx = StdLoad::TypeU32(fScope, 2), sy = StdLoad::TypeU32(fScope, 2);\n    if (!sx || !sy || sx > 255 || sy > 255) throw std::range_error("Invalid sheet grid");\n    sheets.x = U8(sx); sheets.y = U8(sy);')
            body = replace_once(body, '        TextureInfo *ti = new TextureInfo;', '        auto owned = std::make_unique<TextureInfo>();\n        TextureInfo *ti = owned.get();')
            begin = body.index('        ti->texture = new Bitmap(')
            end = body.index('        ti->texMode',begin)
            body = body[:begin] + '        ti->texture = Bitmap::Manager::FindCreate(Bitmap::reduceNONE, image);\n' + body[end:]
            body = replace_once(body, '        images.Add(i, ti);', '        images.Add(i, ti);\n        owned.release();')
        helpers.append(body)
    (output/'frontend_helpers.cpp').write_text('#include "reign/frontend_draw.hpp"\n#include "reign/frontend_bitmap.hpp"\n#include "control_stdload.hpp"\nnamespace IFace {\n'+'\n'.join(helpers)+'\n}\n')
    s = replace_once(texts['interface/input.h'], 'void Init(HINSTANCE inst, HWND window);',
                     '// Windows startup declaration excluded; native input lifecycle remains separate.')
    for declaration in ['extern U16 eventId;', 'extern Point<S32> mousePos;', 'extern U32 lButton;', 'extern U32 rButton;']:
        s = replace_once(s, declaration, declaration.replace('extern ', 'extern thread_local '))
    (output/'control_input.hpp').write_text(s)
    s = texts['system/array.h']
    s = s[s.index('template <class DATA'):s.index('// pads array data')]
    s = s[:s.rfind('//----------------------------------------------------------------------------')]
    s = s.replace('U32 ALIGNMENT = 0', 'U32 ALIGNMENT')
    (output/'control_array.hpp').write_text(s)
    s = texts['util/utiltypes.h']
    r = s[s.index('template <class TYPE> class Range'):s.index('//\n// NameSpace Ranges')]
    r = r[:r.rfind('///////////////////////////////////////////////////////////////////////////////')]
    s = texts['util/stdload.h'];s = s[s.index('namespace StdLoad'):]
    decl = '\n'.join(x for x in s.splitlines() if re.match(
        r'  (U32 TypeU32|F32 TypeF32|const char\* TypeString|U32 TypeStringCrc|template <class TYPE> void TypePoint\(FScope \*fScope)', x))
    (output/'control_stdload.hpp').write_text('#pragma once\n'+r+'\nnamespace StdLoad {\n'+decl+'\n}\n')
    # Preserve original menu scalar/string/coordinate loading, including defaults
    # and inclusive range validation. No rewritten approximation of the parser.
    source = texts['util/stdload.cpp']
    source = source[source.index('namespace StdLoad'):]
    signatures = re.findall(r'^  (?:U32 TypeU32|F32 TypeF32|const char\* TypeString(?:D)?|U32 TypeStringCrc)\([^\n]+\)', source, re.M)
    signatures += ['  template <> void TypePoint(FScope *fScope, Point<'+t+'> &val)' for t in ['F32','U32','S32']]
    if len(signatures) != 19:
        raise ValueError('Unexpected original scalar loader count')
    bodies = []
    for signature in signatures:
        start = source.index(signature+'\n  {')
        end = source.index('{', start)+1
        depth = 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        bodies.append(source[start:end])
    ranges = re.findall(r'^Range<(?:U32|F32)> const Range<[^\n]+;', texts['util/utiltypes.cpp'], re.M)
    # Modern compilers require explicit specialization for these static members.
    ranges = ['template <> '+r.replace('U32_MIN','0u') for r in ranges]
    (output/'control_stdload.cpp').write_text('#include "control_types.hpp"\n#include "fscope.h"\n#include "control_stdload.hpp"\n'+
        '\n'.join(ranges)+'\nnamespace StdLoad {\n'+'\n\n'.join(bodies)+'\n}\n')
    (output/'manifest.json').write_text(json.dumps({'source_hashes': hashes,
        'translation_units': ['icontrol', 'icroot', 'icbutton', 'icwindow', 'ifvar', 'icsystembutton'],
        'linked': False, 'engine_initialized': False, 'menu_rendered': False}, indent=2)+'\n')
    print('Staged six full original frontend translation units; runtime linkage still required')

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('upstream', type=Path);p.add_argument('output', type=Path)
    a = p.parse_args();prepare(a.upstream, a.output)

