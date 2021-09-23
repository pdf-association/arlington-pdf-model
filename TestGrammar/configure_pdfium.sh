#!/bin/bash
error() { echo -e "\033[1;31m$1\033[0m"; }
note() { echo -e "\033[1;36m$1\033[0m"; }
info() { echo -e "\033[1;35m$1\033[0m"; }
success() { echo -e "\033[1;32m$1\033[0m"; }
wait() { read -n 1 -s -r -p "$1"; echo ""; }

applyPatch () { 
  patch=$1
  git apply --whitespace=fix "$patch"
  git add --force .
  git commit -m "$patch"
}

exit_if_error() {
  local exit_code=$1
  shift
  [[ $exit_code ]] &&               # do nothing if no error code passed
    ((exit_code != 0)) && {         # do nothing if error code is 0
      printf 'ERROR: %s\n' "$@" >&2 # we can use better logging here
      exit "$exit_code"             # we could also check to make sure
                                    # error code is numeric when passed
    }
}

# update git repository to specified commit, exluding the specific number or patches
update_git_repo() {  
  REPO_UPDATED=0
  local REPO=$1     # repository name (e.g "boost")
  local GIT=$2      # git repository url
  local BRANCH=$3   # git branch name
  local TARGET=$4   # target commit to reset to
  local PATCHES=$5  # number of patches - must be a number
  local PATCH_PATH=$6     # relative path to patch from the patched git repository
  local RET=0
  if [ -f "$(pwd)/$REPO/.git/HEAD" ]; then
    pushd "$(pwd)/$REPO"
    REMOTE_URL=$(git config --get remote.origin.url)
    popd
    info "$REPO found ... checking remote url $REMOTE_URL"
    if [[ ! "$REMOTE_URL" == "$GIT" ]]; then
      info "$REPO url has changed... deleting"
      # remote url changed, delete repository
      rm -rf "$(pwd)/$REPO"
    fi
  fi

  if [ ! -f "$(pwd)/$REPO/.git/HEAD" ]; then
    info "$REPO not found ... cloning"
    git clone $GIT
    REPO_UPDATED=1
  fi

  pushd "$(pwd)/$REPO"
  local COMMIT=$(git rev-list --max-count=1 --skip=$PATCHES HEAD)
  if [[ ! "$COMMIT" == "$TARGET"* ]]; then
    info "SHA-1 different update required to $BRANCH commit: $TARGET"
    git reset --hard $TARGET
    git pull
    git checkout $BRANCH
    git reset --hard $TARGET
    git submodule init
    git submodule update

    if [ $PATCHES -gt 0 ]; then
      for i in `seq 1 $PATCHES`; do
        echo "apply patch $PATCH_PATH/$REPO-000$i.patch"
        applyPatch $PATCH_PATH/$REPO-000$i.patch
      done
    fi
    REPO_UPDATED=1
  else
    success "up to date"
  fi  
  popd # "./REPO"
}

# pdfium
note "downloading pdfium"
update_git_repo "pdfium" "https://pdfium.googlesource.com/pdfium" "master" "ac8b8c43685f25d17505491f92e9b5302cae1d98"
pushd pdfium

# depot tools
note "downloading depot tools"
mkdir utils
pushd utils
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
pushd depot_tools

#configure for platform
export PATH="$(pwd):$PATH"
export DEPOT_TOOLS_WIN_TOOLCHAIN=0
note "configuring pdfium"
popd
popd
mkdir build
popd
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync --verbose

# libjpeg_turbo patchs
note "patching libjpeg_turbo"
pushd pdfium/third_party
note "pdfium/third_party/libjpeg_turbo patch"
update_git_repo "libjpeg_turbo" "https://chromium.googlesource.com/chromium/deps/libjpeg_turbo.git" "master" "d5148db386ceb4a608058320071cbed890bd6ad2" 2 ../../../patches
popd 
