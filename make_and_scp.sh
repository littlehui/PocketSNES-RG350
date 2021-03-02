##make -f Makefile.rg350 clean
make -f Makefile
./opk/make_opk.sh
scp -r ./opk/PocketSNES.opk root@10.1.1.2:/media/sdcard/apps/PocketSNES_littlehui_new.opk
