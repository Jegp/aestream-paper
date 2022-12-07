import time

import tqdm

import aestream
import norse
import torch


def benchmark(filename, device):
    # Create horizontal and vertical edge detectors
    kernel_size = 9
    gaussian = torch.sigmoid(torch.linspace(-10, 10, kernel_size + 1))
    kernel = (gaussian.diff() - 0.14).repeat(kernel_size, 1)
    kernels = torch.stack((kernel, kernel.T))
    convolution = torch.nn.Conv2d(1, 2, kernel_size, padding=12, bias=False, dilation=3)
    convolution.weight = torch.nn.Parameter(kernels.unsqueeze(1).to(device))

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

    frames = 0
    ts = []
    with aestream.FileInput(
        filename, (346, 260), device=device, ignore_time=False
    ) as stream:
        with torch.no_grad():
            while stream.is_streaming():
                tensor = stream.read()
                t_before = time.time_ns()
                tensor = tensor.reshape(1, 1, 346, 260)
                if not tensor.device == device:
                    tensor = tensor.to("cuda")
                out, state = net(tensor, state)
                frames += 1
                t_after = time.time_ns()
                ts.append(t_after - t_before)
                t_before = t_after
    return frames, ts


def benchmark_file_single(filename, n, device):
    frames = []
    ts = []
    benchmark(filename, device=device)  # Warmup
    for i in tqdm.tqdm(range(n)):
        f, t = benchmark("aestream/example/davis.aedat4", device=device)
        ts.append(torch.tensor(t).float().mean())
        frames.append(torch.tensor(f))
    ts = torch.stack(ts)
    frames = torch.stack(frames).float()
    return ts.mean(), ts.std(), frames.mean(), frames.std(), device


def benchmark_file(filename, n, which="both"):
    # CPU
    r = []
    if which == "both" or which == "cpu":
        r.append(benchmark_file_single(filename, n, "cpu"))

    # GPU
    if which == "both" or which == "gpu":
        r.append(benchmark_file_single(filename, n, "cuda"))

    return r


def save_result(results_file, row, filename):
    with open(results_file, "a") as results:
        results.write(
            f"{filename},{','.join([(x if isinstance(x, str) else str(x.item())) for x in row])}\n"
        )


if __name__ == "__main__":
    n = 128
    results_file = "gpu_results.csv"
    # files = ["aestream/example/davis.aedat4", "cam0197.aedat4", "cam0213.aedat4"]
    files = ["cam0197.aedat4"]
    for file in files:
        print(f"Benchmarking {file}")
        rows = benchmark_file(file, n, which="gpu")
        for row in rows:
            save_result(results_file, row, file)
