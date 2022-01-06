import urllib.request
import tarfile
import zipfile
import json
import os
import sys
import hashlib
import shutil
import platform
import subprocess

def findFileName(dep) :
  url = dep["url"]
  prevEnd = len(url)
  curEnd = url.rfind('/')
  while curEnd >= 0:
    curItem = url[curEnd + 1 : prevEnd]
    if curItem.find('.') > 0:
      return curItem
    prevEnd = curEnd
    curEnd = url.rfind('/', 0, curEnd)
  return dep["name"]

def computeFileHash(file):
  BUF_SIZE = 65536
  sha1 = hashlib.sha256()
  while True:
    data = file.read(BUF_SIZE)
    if not data:
      break
    sha1.update(data)
    
  return sha1.hexdigest()
  
def unzipLLVM(outPath, packageFileName):
  #Thanks LLVM for publishing an exe for windows binaries.
  septZPath = os.path.join("C:", "Program Files", "7-Zip", "7z.exe")
  extractDir = outPath + "_temp"
  res = subprocess.run([septZPath, "x", "-o" + extractDir, packageFileName])
  if res.returncode != 0:
    print("Failed to extract llvm to " + extractDir)
    return False
  os.mkdir(outPath)
  libDir = os.path.join(extractDir, "_Ÿ€")
  for dir in os.listdir(libDir):
    curDir = os.path.join(libDir, dir)
    shutil.move(curDir, outPath)
  shutil.rmtree(extractDir)
  return True

def main() -> int:
  #if len(sys.argv) < 2:
  #  raise "Missing build folder argument"
  
  #buildFolder = sys.argv[1]

  eXlDir = os.path.dirname(os.path.abspath(__file__))
  packageDir = os.path.join(eXlDir, "package")
  if not os.path.exists(packageDir):
    os.mkdir(packageDir)
  
  dependencyFilePath = os.path.join(eXlDir, "dependencies.json")
  dependencyFile = open(dependencyFilePath, "r")
  dependencies = json.load(dependencyFile)
  
  for dep in dependencies:
  
    if "platform" in dep and platform.system() != dep["platform"]:
      continue
  
    fileName = findFileName(dep)
    mustUpdate = True
    packageFileName = os.path.join(packageDir, fileName)
    outPath = os.path.join(packageDir, dep["name"])
    
    hasHash = "hash" in dep
    
    hashFile = outPath + ".sha256"
    if os.path.exists(outPath) and hasHash and os.path.exists(hashFile):
      hashStr = open(hashFile, "r").read()
      if hashStr == dep["hash"]:
        print(dep["name"] + " up to date")
        mustUpdate = False
        continue
      else :
        print("Different hash for " + dep["name"] + "\nCurrent : " + hashStr + "\nExpected : " + dep["hash"] + "\nRedownloading")
    
    if os.path.exists(outPath):
      shutil.rmtree(outPath)
      
    if os.path.exists(hashFile):
      os.remove(hashFile)
    
    print("Downloading " + dep["name"] + "\n")
    with urllib.request.urlopen(dep["url"]) as data:
      packageFile = open(packageFileName, 'wb')
      packageFile.write(data.read())
      packageFile.close()
    packageFile = open(packageFileName, 'rb')
    fileHash = computeFileHash(packageFile)
    packageFile.close()
    
    if hasHash and fileHash != dep["hash"]:
      print("WARNING : Library " + dep["name"] + " has unexpected hash " + fileHash + ", skipping")
      
    print("Unzipping " + dep["name"] + "\n")
    if zipfile.is_zipfile(packageFileName):
      zipfile.ZipFile(packageFileName).extractall(outPath)
    elif tarfile.is_tarfile(packageFileName):
      tarfile.open(packageFileName).extractall(outPath)
    elif packageFileName.endswith("LLVM-13.0.0-win64.exe"):
      if not unzipLLVM(outPath, packageFileName):
        continue
    else :
      print("ERROR : Unrecognized archive format for file " + packageFileName)
      continue
      
    if "patches" in dep:
      patchFailed = False
      for patch in dep["patches"]:
        fileToPatch = os.path.join(outPath, patch["file"])
        patchFile = os.path.join(eXlDir, patch["patch"])
        res = subprocess.run(["patch", "-p1", fileToPatch, patchFile])
        if res.returncode != 0:
          patchFailed = True
          print("Failed to patch file " + fileToPatch)
          break
      if patchFailed:
        continue
    
    open(hashFile, "w").write(fileHash)
    os.remove(packageFileName)
  
  return 0

if __name__ == '__main__':
  exit(main())