from platform import machine
from utils import execute


execute('python3 $PCOMP_DEVROOT/tests/bin/tasm.py')
execute('python3 $PCOMP_DEVROOT/tests/bin/tdisasm.py')
execute('python3 $PCOMP_DEVROOT/tests/bin/tasmroundtrip.py')
execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/vm -e INTERPRETER')

match machine():
    case 'arm64' | 'aarch64':
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/jit -e INTERPRETER')
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/jit -e AArch64JIT')
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/vm -e AArch64JIT')
    case 'amd64' | 'x86_64':
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/jit -e INTERPRETER')
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/jit -e x86_64JIT')
        execute('python3 $PCOMP_DEVROOT/tests/bin/tvm.py -r tests/data/vm -e x86_64JIT')
    case _:
        pass
