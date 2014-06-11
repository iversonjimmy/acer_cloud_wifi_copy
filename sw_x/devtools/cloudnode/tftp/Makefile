
CN_ROOT ?= /home/berndt/trees/cn

CN_OS = $(CN_ROOT)/cnos/CloudNode
CN_OUTPUT = $(CN_OS)/output
CN_IMGS = $(CN_OUTPUT)/images
CN_KERNEL = $(CN_IMGS)/zImage-Acer
CN_BOOTFS = $(CN_OUTPUT)/bootfs
CN_ROOTFS = $(CN_OUTPUT)/rootfs
CN_DATAFS = $(CN_OUTPUT)/datafs

ROOT_TGZ = root.tgz
ROOT_DIR = root

# nothing to do on default;

default:

help:
	@echo "help - print this help"
	@echo "images - copy latest images to ."
	@echo "root - build root fs for HD installation"

# save latest build images to .

boot.img: $(CN_IMGS)/boot.img
	cp $< $@ && chmod 644 $@
system.img: $(CN_IMGS)/system.img
	cp $< $@ && chmod 644 $@
data.img: $(CN_IMGS)/data.img
	cp $< $@ && chmod 644 $@

images: boot.img system.img data.img

.PHONY: images

# make root for disk installation;
# also fix permissions, uid, gid

$(ROOT_TGZ): boot.img system.img data.img
	rm -rf $(ROOT_DIR)
	mkdir -p $(ROOT_DIR)
	rsync -qavH $(CN_BOOTFS)/. $(ROOT_DIR)/.
	rsync -qavH $(CN_ROOTFS)/. $(ROOT_DIR)/usr/.
	rsync -qavH $(CN_DATAFS)/. $(ROOT_DIR)/etc/.
	cp boot.img $(ROOT_DIR)/.
	tar czf $@ -C $(ROOT_DIR) --owner=root --group=root .
	chmod 644 $@

# make all for root;

root: boot.img $(ROOT_TGZ)

.PHONY: root

