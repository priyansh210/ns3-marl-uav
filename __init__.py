import logging
import os
import sys

logger = logging.getLogger(__name__)
if os.getenv("NS3_HOME") is None:
    logger.error("NS3_HOME environment variable not set.")
    sys.exit(1)
else:
    NS3_HOME = str(os.getenv("NS3_HOME"))
