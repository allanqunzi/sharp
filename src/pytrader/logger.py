import datetime
import logging

def createLogger(name):
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)

    #sh = logging.StreamHandler()
    fh = logging.FileHandler(name + '_' + datetime.date.today().strftime("%Y-%m-%d"))
    #sh.setLevel(logging.DEBUG)
    fh.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    #sh.setFormatter(formatter)
    fh.setFormatter(formatter)

    #logger.addHandler(sh)
    logger.addHandler(fh)
    return logger