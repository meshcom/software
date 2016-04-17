#!/usr/bin/python

import sys
import pycurl
from StringIO import StringIO
from os import walk

print len(sys.argv)

if len(sys.argv) < 3:
	print "Please use uploader as: uploadTest.py IP_ADDRESS CODE_NAME.BIN"
else:
	buffer = StringIO()
	c = pycurl.Curl()
	c.setopt(c.URL, str(sys.argv[1]) + "/edit")
	f = []
	path = "data\\"
	for (dirpath, dirnames, filenames) in walk(path):
		f.extend(filenames)
		break
	for file in f:
		print path+file
		c.setopt(c.HTTPPOST, [
			('fileupload', (
				# upload the contents of this file
				c.FORM_FILE, path+file,
			)),
		])
		c.perform()

	print sys.argv[2]
	c.setopt(c.URL, str(sys.argv[1]) + "/update")
	c.setopt(c.HTTPPOST, [
		('fileupload', (
			# upload the contents of this file
			c.FORM_FILE, sys.argv[2],
		)),
	])
	c.perform()
	c.close()