import aestream
import time
import matplotlib.pyplot as plt


tensors = []

with aestream.FileInput("cam0197.aedat4", (346, 260)) as f:
    for i in range(50):
        time.sleep(0.3)
    for i in range(50):
        time.sleep(0.1)
        t = f.read()
        t = t.bool().float()
        tensors.append(t)


for i, t in enumerate(tensors):
    plt.figure(figsize=(6, 4), dpi=300)
    plt.imshow(t.T, cmap="gray")
    plt.savefig(f"img/{i}.png", dpi=300)
