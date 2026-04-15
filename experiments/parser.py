import re
from lab.parser import Parser
from enum import Enum
import os

def error(content, props):
    props["error"] = not content == ''

def to_str(content, props):
    props["error"] = str(props["error"])
    if props["error"]: return
    #. Add rest of stuff here

class FIParser(Parser):
    def __init__(self):
        super().__init__()
        self.add_function(error, file = 'run.err') #. keep first
        self.add_function(to_str) #. keep last, stringyfies thigs that shoulnd't be summed