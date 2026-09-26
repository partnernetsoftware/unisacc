import re
import sys
import _check
sys.exit(_check.main(lambda l: not re.match(r"\s*#\s*(include|define)\b", l)))
