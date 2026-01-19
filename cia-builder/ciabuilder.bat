./bannertool.exe makebanner -i ../meta/banner.png -a ../meta/banner.wav -o banner.bnr
./bannertool.exe makesmdh -s "aurorachat" -l "A chatting app for the 3DS inspired by hbchat" -p "mii-man and other contributors" -i ../meta/icon.png  -o icon.icn
./makerom.exe -f cia -o ../aurorachat.cia -DAPP_ENCRYPTED=false -rsf app.rsf -target t -exefslogo -elf ../aurorachat.elf -icon icon.icn -banner banner.bnr
echo Finished! CIA has been built!
