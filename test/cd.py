import subprocess

# Execute a command
result = subprocess.run(['script', '-q', '-c', 'gcc -o run_array sample.c'], capture_output=True, text=True)

# Get the exit code
exit_code = result.returncode

# Get the standard output
stdout = result.stdout

# Get the standard error
stderr = result.stderr

# Print the results
print(f"Exit Code: {exit_code}")
print(f"Standard Output:\n{stdout}")
print(f"Standard Error:\n{stderr}")

# Check if the command was successful
if exit_code == 0:
    print("Command executed successfully.")
else:
    print("Command failed.")
