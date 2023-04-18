.PHONY: all
all: make-imago make-treestor make-goat3d make-mikmod

.PHONY: clean
clean: clean-imago clean-treestor clean-goat3d clean-mikmod

.PHONY: imago
imago: make-imago

.PHONY: make-imago
make-imago:
	cd imago; $(MAKE)

.PHONY: clean-imago
clean-imago:
	cd imago; $(MAKE) clean

.PHONY: treestor
treestor: make-treestor

.PHONY: make-treestor
make-treestor:
	cd treestor; $(MAKE)

.PHONY: clean-treestor
clean-treestor:
	cd treestor; $(MAKE) clean

.PHONY: goat3d
goat3d: make-goat3d

.PHONY: make-goat3d
make-goat3d:
	cd goat3d; $(MAKE)

.PHONY: clean-goat3d
clean-goat3d:
	cd goat3d; $(MAKE) clean

.PHONY: mikmod
mikmod: make-mikmod

.PHONY: make-mikmod
make-mikmod:
	cd mikmod; $(MAKE)

.PHONY: clean-mikmod
clean-mikmod:
	cd mikmod; $(MAKE) clean
