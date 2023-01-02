import aestream
import time
import matplotlib.pyplot as plt

import torch
import norse

tensors = []

with aestream.FileInput("cam0197.aedat4", (346, 260), device="cuda") as f:
    time.sleep(3)

    # Create horizontal and vertical edge detectors
    kernel_size = 9
    gaussian = torch.sigmoid(torch.linspace(-10, 10, kernel_size + 1))
    kernel = (gaussian.diff() - 0.14).repeat(kernel_size, 1)
    kernels = torch.stack((kernel, kernel.T))
    convolution = torch.nn.Conv2d(1, 2, kernel_size, padding=12, bias=False, dilation=3)
    convolution.weight = torch.nn.Parameter(kernels.unsqueeze(1).to("cuda"))

    # Create Norse network
    # - One refractory cell to inhibit pixels
    # - One convolutional edge-detection layer
    net = (
        norse.torch.SequentialState(
            norse.torch.LIFRefracCell(),
            convolution,
        )
        .eval()
        .to("cuda")
    )
    state = None  # Start with empty state

    frames = []
    edges = []
    n = 0
    with aestream.FileInput(
        "cam0197.aedat4",
        (346, 260),
        device="cuda",
        ignore_time=False,
    ) as stream:
        time.sleep(12)
        stream.read()
        with torch.no_grad():
            with torch.no_grad():
                while stream.is_streaming() and n < 1000:
                    time.sleep(0.01)
                    tensor = stream.read().reshape(1, 1, 346, 260).byte().float()
                    out, state = net(tensor, state)
                    frames.append(tensor.cpu())
                    edges.append(out.cpu())
                    n += 0
            torch.save((frames, edges), "data.dat")
