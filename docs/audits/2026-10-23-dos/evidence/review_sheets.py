from pathlib import Path
from PIL import Image,ImageDraw
names='''followup-pipe-clobber re-full0-redirection re-full0-pipe-empty re-full0-copy-error suite4-B063-batch-long suite4-B060-batch-args
suite4-B057-cross-drive-tools advanced-driveutils-failures followup-cd-tree-drive followup-commands-env suite2-B032-BLOCKED followup-find-quotes-exit
followup-sort-column-cap suite3-B034-sort-limits suite3-B039-time-values suite3-B038-date-formats suite3-B042-builtin-gaps suite3-B036-echo
advanced-keyboard-interactive-expansion suite3-B045-label suite2-B001-dir-paths suite2-B003-dir-wide suite2-B023-del-directory suite2-B030-redir-readonly
advanced-misc-errorlevel-shift advanced-fdisk-partitions prog-garbage-result prog-mz16-result app123-launch appword-launch
interactive-batch-after-break interactive-f3-after-break interactive-dir-page-break re-dirty-rescan re-fat16-completed-read advanced-format-system-switch
stock-cls stock-dir interactive-template-f1-insert-delete interactive-template-f2-f3-f5-esc advanced-keyboard-caps advanced-keyboard-127-128'''.split()
for batch in range(0,len(names),6):
 im=Image.new('RGB',(1920,1016),'white');dr=ImageDraw.Draw(im)
 for j,n in enumerate(names[batch:batch+6]):
  x=j%3*640;y=j//3*508;im.paste(Image.open('shots/'+n+'.png'),(x,y+28));dr.text((x+4,y+7),n,fill='black')
 im.save('evidence/review-%02d.png'%(batch//6))
