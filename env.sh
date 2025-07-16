source env/python/bin/activate

export PATH=~/tools/bin:$PATH
export PATH=/opt/homebrew/opt/binutils/bin:$PATH
export PCOMP_DEVROOT=~/work/pcomp
export PYTHONDONTWRITEBYTECODE=1
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PCOMP_DEVROOT/vm/build
export DYLD_FALLBACK_LIBRARY_PATH=$DYLD_FALLBACK_LIBRARY_PATH:$PCOMP_DEVROOT/vm/build

alias make='make -j 4'
alias lldb='/Library/Developer/CommandLineTools/usr/bin/lldb'
