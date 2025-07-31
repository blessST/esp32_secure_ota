import sys, hashlib, json
version=sys.argv[1]
fw=sys.argv[2]
meta=sys.argv[3]
url=f"https://your-server.com/{fw}"
sha=hashlib.sha256(open(fw,"rb").read()).hexdigest()
json.dump({"version":version,"url":url,"sha256":sha},open(meta,"w"),indent=2)
