#!/usr/bin/python3

import subprocess
import time
import pagemap

dumper_path = '../dumper/dumper'
extractor_path = '../extractor/extractor'
modifier_path = '../modifier/modifier'

# Detect GPU architecture
def is_blackwell_gpu():
    try:
        result = subprocess.run(['nvidia-smi', '--query-gpu=gpu_name', '--format=csv,noheader'],
                              capture_output=True, text=True, timeout=5)
        gpu_name = result.stdout.strip().upper()
        return 'GB2' in gpu_name or 'BLACKWELL' in gpu_name
    except:
        return False

target_va = 0x700000000000
dummy_va = 0x7f0000000000
code_pa = 0x01400000

#===============================================================================
# modify PTE for target_va to make it point to the dummy_va's page frame
#===============================================================================
sudo = subprocess.Popen(['sudo', 'echo', ''])
sudo.wait()

# dump GPU memory and retrieve the pagemap from the dump
a_out = subprocess.Popen(['./l1-itlb-cap', hex(dummy_va)])
b_out = subprocess.Popen(['nvidia-smi', '-f', '/dev/null']) 
b_out.wait() # in case the driver warms up very slowly
time.sleep(2)
tmp_path = '/tmp/'
dump = subprocess.Popen([dumper_path, '-b', '0x10000000', '-o', tmp_path + 'dump'])
dump.wait()
pagemap_file = open(tmp_path + 'pagemap', 'w')
extractor_args = [extractor_path, tmp_path + 'dump']
if is_blackwell_gpu():
    extractor_args.append('--blackwell')
extract = subprocess.Popen(extractor_args, stdout=pagemap_file)
extract.wait()
pagemap_file.close()
a_out.kill()
time.sleep(5)

# extract PTEs from the pagemap
ptes = pagemap.retrieve_ptes(tmp_path + 'pagemap')
pte_pa = ptes[target_va][1]
pte_val = pagemap.make_pte_value(ptes[dummy_va][0])

code_va = None
for key, val in ptes.items():
  if val[0] == code_pa:
    code_va = key
    break
print("code is at " + hex(code_va))

a_out = subprocess.Popen(['./l1-itlb-cap', hex(code_va)])
b_out = subprocess.Popen(['nvidia-smi', '-f', '/dev/null']) 
b_out.wait() # in case the driver warms up very slowly
time.sleep(10)
subprocess.Popen(['sudo', modifier_path, hex(pte_pa), hex(pte_val)])
time.sleep(10)

a_out.kill()
if a_out.poll() != None:
  print('eviction successful')
else:
  print('no eviction')


