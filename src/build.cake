###############################################################################
# Copyright (c) Lewis Baker
# Licenced under MIT license. See LICENSE.txt for details.
###############################################################################

import cake.path

from cake.tools import script, env, compiler, project, variant, test

script.include([
  env.expand('${CPPCORO}/lib/use.cake'),
])

#headers = script.cwd([
#  "counted.hpp",
#  ])

sources = script.cwd([
  'main.cpp'
  ])

extras = script.cwd([
  'build.cake',
])

intermediateBuildDir = script.cwd('build')

compiler.addDefine('CPPCORO_RELEASE_' + variant.release.upper())
compiler.addLibrary('m')

objects = compiler.objects(
  targetDir=intermediateBuildDir,
  sources=sources,
)

testExe = compiler.program(
  target=script.cwd('build/main'),
  sources=objects,
)

script.addTarget('testresult', testExe)
