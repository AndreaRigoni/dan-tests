
include $(top_srcdir)/conf/kscripts/build_common.mk
include $(top_srcdir)/conf/kscripts/toolchain.mk
include $(top_srcdir)/conf/kscripts/docker.mk

## ////////////////////////////////////////////////////////////////////////// ##
## /// ACTIVATE HELP TARGET ///////////////////////////////////////////////// ##
## ////////////////////////////////////////////////////////////////////////// ##

@TARGET_SELFHELP@

## /////////////////////////////////////////////////////////////////////////////
## // DIRECTORIES //////////////////////////////////////////////////////////////
## /////////////////////////////////////////////////////////////////////////////

DL   ?= $(DOWNLOAD_DIR)
TMP  ?= $(abs_top_builddir)

${DL} ${TMP}:
	@$(MKDIR_P) $@



## /////////////////////////////////////////////////////////////////////////////
## // RECONFIGURE  /////////////////////////////////////////////////////////////
## /////////////////////////////////////////////////////////////////////////////

.PHONY: reconfigure
reconfigure: ##@miscellaneous re-run configure with last passed arguments
	@ \
	echo " -- Reconfiguring build with following parameters: -----------"; \
	echo $(shell $(abs_top_builddir)/config.status --config);              \
	echo " -------------------------------------------------------------"; \
	echo ; \
	cd '$(abs_top_builddir)' && \
	$(abs_top_srcdir)/configure $(shell $(abs_top_builddir)/config.status --config);
	

print-banner: ##@miscellaneous print logo
	-@ cat $(top_srcdir)/docs/logo.txt

help: print-banner




## ////////////////////////////////////////////////////////////////////////////////
## //  CODAC  DOCKER  /////////////////////////////////////////////////////////////
## ////////////////////////////////////////////////////////////////////////////////

## export DOCKER_ENTRYPOINT ?= $(top_srcdir)/codac/$(CODAC_NAME)_entry.sh