ROOTDIR:=.
INCDIR:=$(ROOTDIR)/include
SRCDIR:=$(ROOTDIR)/src
TESTDIR:=$(ROOTDIR)/test
EXAMPLEDIR:=$(ROOTDIR)/example
MEMCHECK?=n
DEBUG?=n

.PHONY: all
all:
	@cd $(TESTDIR); make DEBUG=$(DEBUG)
	@cd $(EXAMPLEDIR); make DEBUG=$(DEBUG)

.PHONY: test
test:
	@cd $(TESTDIR); make test MEMCHECK=$(MEMCHECK) DEBUG=$(DEBUG)

.PHONY: clean
clean:
	-@rm -f $(ROOTDIR)/*~
	@cd $(TESTDIR); make clean
	@cd $(EXAMPLEDIR); make clean
