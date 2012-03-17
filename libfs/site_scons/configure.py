from SCons.Script import *

def configure(env):
    vars=Variables('build-setup.conf')
    vars.Add('GOOGLE_SPARSEHASH', ARGUMENTS.get('GOOGLE_SPARSEHASH', ''), '')
    vars.Add('LIBCONFIG', ARGUMENTS.get('LIBCONFIG', ''), '')
    vars.Add('RPC', ARGUMENTS.get('RPC', ''), 'socket')
    vars.Update(env)
    vars.Save('build-setup.conf', env)
