import os
import shutil
import subprocess
import sys

def setup_test_env():
    # Clean any old test directories
    cleanup()

    # Create directories
    os.makedirs("test_env/sub1/ignored_dir", exist_ok=True)
    os.makedirs("test_env/sub1/normal_dir", exist_ok=True)
    os.makedirs("test_env/sub2", exist_ok=True)

    # Create files
    with open("test_env/sub1/main.cpp", "w", encoding="utf-8") as f:
        f.write("// Hello main\n")
    
    with open("test_env/sub1/ignored_dir/ignored.cpp", "w", encoding="utf-8") as f:
        f.write("// Hello ignored\n")
        
    with open("test_env/sub1/normal_dir/nested.cpp", "w", encoding="utf-8") as f:
        f.write("// Hello nested\n")
        
    with open("test_env/sub2/another.cpp", "w", encoding="utf-8") as f:
        f.write("// Hello another\n")

def cleanup():
    for path in ["test_env", "combine-txt"]:
        if os.path.exists(path):
            shutil.rmtree(path)

def test_exclusion_logic():
    print("Running Test 1: Exclude nested directory 'ignored_dir'")
    setup_test_env()
    
    # Run combine.exe capturing with UTF-8 encoding
    result = subprocess.run([".\\combine.exe", "test_env", "--exclude", "ignored_dir"], capture_output=True, text=True, encoding="utf-8", errors="ignore")
    
    # Check stdout
    stdout = result.stdout
    print("STDOUT Output:\n", stdout)
    
    assert "ignored.cpp" not in stdout, "Error: ignored.cpp found in stdout"
    assert "nested.cpp" in stdout, "Error: nested.cpp not found in stdout"
    assert "main.cpp" in stdout, "Error: main.cpp not found in stdout"
    assert "another.cpp" in stdout, "Error: another.cpp not found in stdout"
    
    # Check combine-txt output
    assert os.path.exists("combine-txt/sub1.txt"), "Error: sub1.txt not written"
    assert os.path.exists("combine-txt/sub2.txt"), "Error: sub2.txt not written"
    
    with open("combine-txt/sub1.txt", "r", encoding="utf-8") as f:
        sub1_content = f.read()
        
    assert "ignored.cpp" not in sub1_content, "Error: ignored.cpp written to sub1.txt"
    assert "nested.cpp" in sub1_content, "Error: nested.cpp not found in sub1.txt"
    assert "main.cpp" in sub1_content, "Error: main.cpp not found in sub1.txt"
    
    print("Test 1 passed successfully!")

def test_multiple_exclusions():
    print("\nRunning Test 2: Exclude 'ignored_dir' and immediate subdirectory 'sub2'")
    setup_test_env()
    
    # Run combine.exe with comma-separated list
    result = subprocess.run([".\\combine.exe", "test_env", "--exclude", "ignored_dir,sub2"], capture_output=True, text=True, encoding="utf-8", errors="ignore")
    stdout = result.stdout
    print("STDOUT Output:\n", stdout)
    
    assert "sub2" not in stdout, "Error: sub2 processed when excluded"
    assert "another.cpp" not in stdout, "Error: another.cpp found in stdout"
    assert "ignored.cpp" not in stdout, "Error: ignored.cpp found in stdout"
    assert "nested.cpp" in stdout, "Error: nested.cpp not found in stdout"
    
    assert os.path.exists("combine-txt/sub1.txt"), "Error: sub1.txt not written"
    assert not os.path.exists("combine-txt/sub2.txt"), "Error: sub2.txt written when excluded"
    
    print("Test 2 passed successfully!")

def test_multiple_options():
    print("\nRunning Test 3: Exclude 'ignored_dir' and 'sub2' with multiple --exclude flags")
    setup_test_env()
    
    # Run combine.exe with multiple flags
    result = subprocess.run([".\\combine.exe", "test_env", "--exclude", "ignored_dir", "--exclude", "sub2"], capture_output=True, text=True, encoding="utf-8", errors="ignore")
    stdout = result.stdout
    
    assert "sub2" not in stdout, "Error: sub2 processed when excluded"
    assert "another.cpp" not in stdout, "Error: another.cpp found in stdout"
    assert "ignored.cpp" not in stdout, "Error: ignored.cpp found in stdout"
    
    print("Test 3 passed successfully!")

if __name__ == "__main__":
    try:
        test_exclusion_logic()
        test_multiple_exclusions()
        test_multiple_options()
        print("\nAll tests passed successfully!")
    finally:
        cleanup()
