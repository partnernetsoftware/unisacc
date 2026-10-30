import re
import sys
import _check
L = re.compile(r"^[A-Za-z_.$][\w.$]*:$|^\s+\S|^\.\w+")
sys.exit(_check.main(lambda l: bool(L.match(l))))
