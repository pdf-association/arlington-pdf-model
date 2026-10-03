# Stub name for docker containers (so can be meaningfully monitored)
DOCKER_NAME := mn-arlington

# Name of main AsciiDoc file for PDFa publication (in YAML file for MN, but needed for AsciiDoctor CLI)
MAIN_PDFA_ADOC := arlington-test.adoc
MAIN_ISO_ADOC  := arlington-test.adoc


# Check for Windows_NT environment variable (cmd.exe/PowerShell)
OS ?= $(shell echo %OS% 2>/dev/null)  # Fetch %OS% from Windows shell
ifeq ($(OS),Windows_NT)
    IS_WINDOWS := 1  # Confirmed Windows (cmd.exe/PowerShell)
		RM:=del /F/Q
		RM_RECURSIVE:=del /F/S/Q
		RMDIR:=rmdir /Q/S
		CP:=copy /Y
		SEP:=\\
		PWD:=cd
else
    # Not Windows cmd.exe; check via `uname -s` (Unix-like shells)
    UNAME_S := $(shell uname -s 2>/dev/null)  # Get OS name (e.g., Linux, Darwin)
		RM:=rm
		RM_RECURSIVE:=rm -rf
		RMDIR:=rm -rf
		CP:=cp
		SEP:=/
		PWD:=pwd
    
    # Detect Unix-like systems (Linux/macOS)
    ifeq ($(UNAME_S),Linux)
        IS_UNIX := 1
    else ifeq ($(UNAME_S),Darwin)  # macOS uses "Darwin"
        IS_UNIX := 1
    # Detect Windows subsystems (MINGW64, CYGWIN, etc.)
    else ifneq ($(filter MINGW%,$(UNAME_S)),)  # Git Bash/MinGW
        IS_WINDOWS := 1
    else ifneq ($(filter CYGWIN%,$(UNAME_S)),)  # Cygwin
        IS_WINDOWS := 1
    else
        # Fallback: Assume Unix-like if unrecognized
        IS_UNIX := 1
    endif
endif

# Clean up all outputs that can be re-created. Does NOT delete Gemfile.lock!
# Windows doesn't support wildcards for folder names.
.PHONY: clean
clean:
	-@$(RMDIR) iev relaton
	-@$(RMDIR) _site_diso _site_dpdfa _site_iso _site_pdfa _site
	-@$(RMDIR) _plantuml_images
	-@$(RM_RECURSIVE) *.log
	-@$(RM_RECURSIVE) *.abort 
	-@$(RM_RECURSIVE) *.log.txt
	-@$(RM_RECURSIVE) *.err.*
	-@$(RM_RECURSIVE) *asciidoc*
	-@$(RM_RECURSIVE) docker*.*


# Update the bundle to the latest MN gems
.PHONY: update-bundle
update-bundle:
	$(RM) *.lock
	bundle install


# Build using existing bundled Gems
.PHONY: build-bundle
build-bundle:
	bundle exec metanorma site generate --agree-to-terms metanorma.yml > bundle.log 2>&1
	python3 ../../publication-info/postMN.py ./_site/documents/arlington-test.html


# Build using Docker. SLOW using last stable MN.
# Removes containers once done. Named in case monitoring.
.PHONY: build-docker
build-docker:
	docker pull metanorma/metanorma
	docker run --name $(DOCKER_NAME) --rm --volume .:/metanorma/ -w /metanorma metanorma/metanorma metanorma site generate metanorma.yml --agree-to-terms > docker.log 2>&1
	python3 ../../publication-info/postMN.py ./_site/documents/arlington-test.html

