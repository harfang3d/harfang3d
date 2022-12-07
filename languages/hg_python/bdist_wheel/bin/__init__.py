from .run import run
def __getattr__(name:str):
    return run(name)
