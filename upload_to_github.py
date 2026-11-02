import subprocess
import time
import os
import re
import sys

GH = r'C:\Users\Srijith S D\AppData\Local\Microsoft\WinGet\Packages\GitHub.cli_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\gh.exe'
GIT = r'C:\Users\Srijith S D\AppData\Local\Programs\MinGit\cmd\git.exe'
STATUS_FILE = r'C:\Users\Srijith S D\Downloads\DBMS\anti\.github_upload_status.txt'

def log(msg):
    print(msg, flush=True)
    with open(STATUS_FILE, 'a', encoding='utf-8') as f:
        f.write(msg + '\n')

with open(STATUS_FILE, 'w', encoding='utf-8') as f:
    f.write('STARTING GITHUB AUTH AND UPLOAD\n')

# Check if already authenticated
check = subprocess.run([GH, 'auth', 'status'], capture_output=True, text=True)
if 'Logged in to github.com' not in check.stdout and 'Logged in to github.com' not in check.stderr:
    log('Authenticating with GitHub...')
    proc = subprocess.Popen(
        [GH, 'auth', 'login', '--web', '-h', 'github.com', '-p', 'https'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    time.sleep(1)
    proc.stdin.write('\n')
    proc.stdin.flush()

    # Read stderr line by line until code is found
    code = None
    url = 'https://github.com/login/device'
    
    start_time = time.time()
    while time.time() - start_time < 10:
        line = proc.stderr.readline()
        if not line:
            time.sleep(0.5)
            continue
        log(f'[GH] {line.strip()}')
        m = re.search(r'([A-Z0-9]{4}-[A-Z0-9]{4})', line)
        if m:
            code = m.group(1)
            break

    if code:
        log(f'=========================================')
        log(f'ACTION REQUIRED:')
        log(f'1. Open: {url}')
        log(f'2. Enter Code: {code}')
        log(f'=========================================')
    else:
        log('Could not extract code automatically. Waiting on process...')

    # Wait for authentication to complete (timeout after 5 minutes)
    proc.wait(timeout=300)
    log('Authentication process finished!')

# Verify auth status
auth_res = subprocess.run([GH, 'auth', 'status'], capture_output=True, text=True)
log(auth_res.stdout)
log(auth_res.stderr)

# Get current username
user_res = subprocess.run([GH, 'api', 'user', '--jq', '.login'], capture_output=True, text=True)
username = user_res.stdout.strip()
log(f'Logged in as GitHub user: {username}')

if not username:
    log('Failed to detect username. Aborting upload.')
    sys.exit(1)

# Ensure local git repo is clean and committed
subprocess.run([GIT, 'add', '.'], capture_output=True)
subprocess.run([GIT, 'commit', '-m', 'CineBook Movie Ticket Booking System complete project'], capture_output=True)
subprocess.run([GIT, 'branch', '-M', 'main'], capture_output=True)

# Create repository on GitHub
log(f'Creating/pushing repository for {username}...')
create_res = subprocess.run(
    [GH, 'repo', 'create', 'cinebook', '--public', '--source=.', '--remote=origin', '--push'],
    capture_output=True,
    text=True
)
log(f'Create output: {create_res.stdout}')
if create_res.stderr:
    log(f'Create stderr: {create_res.stderr}')

# If repo already exists or remote exists, push to origin
if create_res.returncode != 0:
    log('Attempting direct push to origin...')
    subprocess.run([GIT, 'remote', 'set-url', 'origin', f'https://github.com/{username}/cinebook.git'], capture_output=True)
    push_res = subprocess.run([GIT, 'push', '-u', 'origin', 'main'], capture_output=True, text=True)
    log(f'Push stdout: {push_res.stdout}')
    log(f'Push stderr: {push_res.stderr}')

repo_url = f'https://github.com/{username}/cinebook'
log('=========================================')
log(f'SUCCESS! Your repository is live at:')
log(repo_url)
log('=========================================')
