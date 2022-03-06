import numpy as np
from matplotlib import pyplot as plt

def binfile_to_np_array(filename):
    # Open a binary file formatted as btnearme_history and return its contents as a numpy array 
    with open(filename, "rb") as history:
        lines = history.readlines()
    data = np.zeros(len(lines), dtype=int)
    for i, line in enumerate(lines):
        hours = int(line[1:3])
        minutes = hours * 60 + int(line[4:6])
        seconds = minutes * 60 + int(line[7:9])
        nanoseconds = seconds * 1E9 + int(line[10:19])
        data[i] = nanoseconds
    return data

history_data = binfile_to_np_array("btnearme_history")
expected_data = binfile_to_np_array("btnearme_expected")

error_data = history_data - expected_data

plt.title("BTnearMe execution time delay")
plt.ylabel("Delay in BTnearMe execution (nanosec)")
plt.xlabel("Time (search interval = 0.1 sec)")
plt.plot(error_data)
plt.plot(np.ones_like(error_data) * error_data.mean(), color="yellow", linestyle="dashed")
plt.savefig("btnearme_delay.png", dpi=200)
