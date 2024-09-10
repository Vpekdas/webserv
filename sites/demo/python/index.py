import os

print("Content-Type: text/html", end="\r\n")
print("", end="\r\n")

#print(os.environ)

if "Mozilla" in os.environ['HTTP_USER_AGENT']:
	print("Hello from firefox !")
