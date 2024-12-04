set +x
dd if=/dev/zero of=test.img bs=512 count=4096
mkfs.msdos test.img
echo here
mcopy -i test.img build/nux/apxh/efi/apxh.efi ::apxh.efi
mcopy -i test.img build/kern/machina ::kernel.elf
mcopy -i test.img build/tests/test/test ::user.elf
