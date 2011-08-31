import os, subprocess, string, re, sys, signal
import Queue
import testfw
from xml.etree.ElementTree import ElementTree
import xml
import testfw.integration_test


def addIntegrationTest(env, integration_test):
    env.Append(INTEGRATION_TESTS = [integration_test])



def runIntegrationTests(source, target, env):
    results = []
    for itest in env['INTEGRATION_TESTS']:
        itest.run()
