import sys
import audiosync


print("Running module")
ret = audiosync.get_lag(sys.argv[1])
print(f"Returned value: {ret}")
