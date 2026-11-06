# Helper macros to convert spaces into question marks and back again
e := 
sp := $(e) $(e)
qs = $(subst ?,$(sp),$1)
sq = $(subst $(sp),?,$1)

# Get name of this Makefile (avoid getting word 0 and a starting space)
makefile_name := $(wordlist 1,1000,$(MAKEFILE_LIST))

# Get path of this Makefile
makefile_path := $(call qs,$(dir $(call sq,$(abspath $(call sq,$(makefile_name))))))

# Get path where the Application is
application_path := $(call qs,$(abspath $(call sq,$(makefile_path)../..)))

.PHONY: clean assets all update_bitmap_lists

ifneq ($(words $(makefile_path))$(words $(MAKEFILE_LIST)),11)
all clean assets:
$(error Spaces not allowed in path)
else

ADDITIONAL_SOURCES := ../Core/Src/moon.c
ADDITIONAL_INCLUDE_PATHS := ../Core/Inc
ADDITIONAL_LIBRARY_PATHS := 
ADDITIONAL_LIBRARIES :=
export ADDITIONAL_SOURCES ADDITIONAL_INCLUDE_PATHS ADDITIONAL_LIBRARY_PATHS ADDITIONAL_LIBRARIES

# Keep the bitmap cache lists of the views in sync with the screens (Generate Code / Run Simulator).
# A failure (e.g. python not found) does not stop the build.
all assets: update_bitmap_lists
update_bitmap_lists:
	-@python "$(application_path)/tools/update_bitmap_lists.py"

all: $(filter assets,$(MAKECMDGOALS))
all assets: $(filter clean,$(MAKECMDGOALS))
all clean assets:
	@$(MAKE) -r -f generated/simulator/gcc/Makefile -s $(MFLAGS) $@ -C "$(application_path)"
endif

