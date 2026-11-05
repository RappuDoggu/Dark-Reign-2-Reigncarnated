#!/usr/bin/env python3
"""Edit an authored 30-second draft from private real-engine PPM/timing capture.
Needs host FFmpeg with libx264/drawtext, ffprobe and DejaVu fonts. No runtime
assets are read by this editor and raw footage remains outside the checkout.
"""
import argparse,csv,hashlib,json,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--capture',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
p.add_argument('--source-revision',required=True)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.output.resolve();capture=a.capture.resolve()
if out==repo or repo in out.parents:p.error('Keep private captures and edit output outside the source tree')
out.mkdir(parents=True,exist_ok=True)
ffmpeg=shutil.which('ffmpeg');ffprobe=shutil.which('ffprobe')
font=Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf');bold=font.with_name('DejaVuSans-Bold.ttf');mono=font.with_name('DejaVuSansMono.ttf')
if not ffmpeg or not ffprobe or not all(x.is_file() for x in (font,bold,mono)):p.error('Install host FFmpeg/ffprobe and DejaVu fonts')
rows=list(csv.DictReader((capture/'frames.csv').open()))
if len(rows)<30:p.error('Need actual frame timing and at least 30 gameplay frames')
lines=['ffconcat version 1.0']
for i,row in enumerate(rows):
 frame=capture/('frame-%05d.ppm'%int(row['frame']))
 if not frame.is_file():p.error('Missing captured frame '+str(frame))
 seconds=float(rows[i+1]['seconds'])-float(row['seconds']) if i+1<len(rows) else 1/15
 if not 0<seconds<2:p.error('Unexpected capture gap; inspect the source footage')
 if "'" in str(frame):p.error('Use a capture directory without quotes')
 lines += ["file '"+str(frame)+"'",'duration %.9f'%seconds]
concat=out/'capture.ffconcat';concat.write_text('\n'.join(lines)+'\n')
texts={}
def label(key,text,size,x,y,color='0xe6eee5',weight='normal',extra=''):
 f=out/(key+'.txt');f.write_text(text+'\n');texts[key]=text
 face=bold if weight=='bold' else mono if weight=='mono' else font
 return f'drawtext=fontfile={face}:textfile={f}:fontsize={size}:fontcolor={color}:x={x}:y={y}'+extra
base='color=c=0x081410:s=1280x720:r=30'
intro=[label('intro_kicker','REAL-TIME STRATEGY / ANDROID',19,86,154,'0xabc5b2','mono'),label('intro_title','REIGNCARNATED',74,80,258,'0xe2efd7','bold'),label('intro_sub','DARK REIGN II, REAWAKENING',25,86,365,'0xa4c37e'),label('intro_draft','TRAILER MOCKUP 01  /  WORK IN PROGRESS',15,86,625,'0x8fa59a','mono')]
intrograph=base+':d=3,drawbox=x=86:y=225:w=110:h=4:color=0xc1a05c:t=fill,drawbox=x=86:y=436:w=1100:h=1:color=0x253c31:t=fill,'+','.join(intro)+',fade=t=in:d=0.3,fade=t=out:st=2.7:d=0.3[v]'
end=[label('end_kicker','DARK REIGN II / ANDROID ARM64',19,86,154,'0xabc5b2','mono'),label('end_title','REIGNCARNATED',74,80,258,'0xe2efd7','bold'),label('end_sub','THE NEXT CHAPTER IS TAKING SHAPE.',24,86,365,'0xa4c37e'),label('end_wip','OFFLINE PROTOTYPE  /  WORK IN PROGRESS',18,86,430,'0xc1a05c','mono'),label('end_foot','Draft uses original-engine host capture and authored placeholder audio.',15,86,625,'0x8fa59a')]
endgraph=base+':d=7,drawbox=x=86:y=225:w=110:h=4:color=0xc1a05c:t=fill,'+','.join(end)+',fade=t=in:d=0.3,fade=t=out:st=6.5:d=0.5[v]'
footlabels=[label('game_header','REIGNCARNATED',20,28,32,'0xe2efd7','bold'),label('game_tag','WIP',17,1210,34,'0xc1a05c','mono'),label('game_foot','ACTUAL ENGINE CAPTURE  /  HOST PREVIEW',14,28,678,'0xa8baac','mono'),label('game_note','ANDROID PORT IN DEVELOPMENT',12,980,681,'0xa8baac','mono')]
for key,text,start,endtime in [('scout','SCOUT',0,6.7),('command','COMMAND',6.7,13.4),('move','MOVE',13.4,20)]:
 footlabels.append(label(key,text,19,28,325,'0xc1a05c','mono',":enable='between(t,%g,%g)'"%(start,endtime)))
gamegraph='[0:v]fps=30,scale=864:648:flags=lanczos,pad=1280:720:208:36:color=0x081410,setsar=1,'+','.join(footlabels)+',fade=t=in:d=0.3,fade=t=out:st=19.7:d=0.3[v]'
def render(path,graph,inputs=(),duration=None):
 cmd=[ffmpeg,'-nostdin','-hide_banner','-loglevel','error','-y',*inputs,'-filter_complex',graph,'-map','[v]']
 if duration:cmd+=['-t',str(duration)]
 subprocess.run(cmd+['-an','-r','30','-c:v','libx264','-preset','medium','-crf','19','-pix_fmt','yuv420p','-movflags','+faststart',str(path)],check=True)
render(out/'intro.mp4',intrograph)
render(out/'gameplay.mp4',gamegraph,['-f','concat','-safe','0','-i',str(concat)],20)
render(out/'outro.mp4',endgraph)
segments=out/'segments.txt';segments.write_text(''.join("file '"+str(out/name)+"'\n" for name in ['intro.mp4','gameplay.mp4','outro.mp4']))
final=out/'Reigncarnated-Trailer-Mockup-01.mp4'
# An authored low-volume pulse bed; no original music, dialogue or combat audio.
tone='0.055*sin(2*PI*55*t)*(0.4+0.6*pow(sin(PI*2*t),8))+0.012*sin(2*PI*110*t)'
subprocess.run([ffmpeg,'-nostdin','-hide_banner','-loglevel','error','-y','-f','concat','-safe','0','-i',str(segments),'-f','lavfi','-i',f"aevalsrc='{tone}':s=48000:d=30",'-af','afade=t=in:d=1,afade=t=out:st=28.5:d=1.5','-map','0:v','-map','1:a','-c:v','copy','-c:a','aac','-b:a','128k','-ac','2','-t','30','-movflags','+faststart',str(final)],check=True)
probe=json.loads(subprocess.check_output([ffprobe,'-v','error','-show_format','-show_streams','-of','json',str(final)]))
video=next(x for x in probe['streams'] if x['codec_type']=='video');audio=next(x for x in probe['streams'] if x['codec_type']=='audio')
assert video['width']==1280 and video['height']==720 and abs(float(probe['format']['duration'])-30)<.1
assert video['codec_name']=='h264' and audio['codec_name']=='aac'
receipt={'source_revision':a.source_revision,'kind':'draft trailer mockup; not an Android gameplay benchmark','seconds':float(probe['format']['duration']),'resolution':[1280,720],'capture':'original engine host GLES; private owned 2 Rush reference map; one AI','captured_frames':len(rows),'capture_elapsed_seconds':float(rows[-1]['seconds']),'simulation_cycles_delta':int(rows[-1]['simulation_cycles'])-int(rows[0]['simulation_cycles']),'soundtrack':'authored placeholder pulse; no original audio','game_data_bundled':False,'public_release_announced':False,'editor_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'captions':texts,'sha256':hashlib.sha256(final.read_bytes()).hexdigest(),'bytes':final.stat().st_size}
(out/'trailer-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
subprocess.run([ffmpeg,'-nostdin','-hide_banner','-loglevel','error','-y','-ss','1.5','-i',str(final),'-frames:v','1',str(out/'poster.png')],check=True)
print(final);print(json.dumps({k:receipt[k] for k in ['seconds','resolution','bytes','sha256']},indent=2))
