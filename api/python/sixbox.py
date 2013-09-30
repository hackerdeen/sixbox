
import urllib2

class Sixbox:

	def __init__(self):
		self.waiting = {}

	def yellow(self, on, immediate=False):
		if on:
			self.waiting['X-Yellow'] = 'On'
		else:
			self.waiting['X-Yellow'] = 'Off'
		if immediate:
			self.update()

	def green(self, on, immediate=False):
		if on:
			self.waiting['X-Green'] = 'On'
		else:
			self.waiting['X-Green'] = 'Off'
		if immediate:
			self.update()

	def buzz(self, immediate=False):
		self.waiting['X-Buzz'] = 'Yes'
		if immediate:
			self.update()

	def update(self):
		request = urllib2.Request("http://[2a00:eb0:100:15::1234]/", None, self.waiting)
		response = urllib2.urlopen(request)
		self.waiting.clear()
		return response.read()


