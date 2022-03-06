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
x_data = list(range(len(error_data)))

plt.title("BTnearMe execution time delay")
plt.ylabel("Delay in BTnearMe execution (nanosec)")
plt.xlabel("Time (search interval = 0.1 sec)")
plt.plot(x_data, error_data, label="data", zorder=2.5)
plt.plot(np.ones_like(error_data) * error_data.mean(), color="yellow", linestyle="dashed", linewidth=1.5, label="mean", zorder=2.5)
plt.fill_between(x_data, error_data.mean() + error_data.std(), error_data.mean() - error_data.std(), color="gold", alpha = 0.25, zorder=2.5, label="standard deviation")
plt.legend()
plt.savefig("btnearme_delay.png", dpi=200)
