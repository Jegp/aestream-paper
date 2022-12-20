import argparse
import time

import tqdm

import aestream
import norse
import torch


def benchmark(filename, device, use_coroutines: bool):
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
    time_run_total = 0
    with aestream.FileInput(
        filename,
        (346, 260),
        device=device,
        ignore_time=False,
        use_coroutines=use_coroutines,
    ) as stream:
        with torch.no_grad():
            time_start = time.time_ns()
            while stream.is_streaming():
                tensor = stream.read()
                t_before = time.time_ns()
                if not tensor.device == device:
                    tensor = tensor.to("cuda", non_blocking=True)
                tensor = tensor.reshape(1, 1, 346, 260)
                out, state = net(tensor, state)
                t_after = time.time_ns()
                frames += 1
                ts.append(t_after - t_before)
                t_before = t_after
            time_run_total = time.time_ns() - time_start
    return frames, ts, time_run_total


def benchmark_file_single(filename, n, device, use_coroutines):
    frames = []
    ts = []
    totals = []
    cs = []
    benchmark(filename, device, use_coroutines)  # Warmup
    for i in tqdm.tqdm(range(n)):
        f, t, total = benchmark(
            filename,
            device=device,
            use_coroutines=use_coroutines,
        )
        cs.append(use_coroutines)
        ts.append(torch.tensor(t).float().mean())
        frames.append(torch.tensor(f))
        totals.append(torch.tensor(total))
    ts = torch.stack(ts)
    frames = torch.stack(frames).float()
    totals = torch.stack(totals).float()
    return (
        ts.mean(),
        ts.std(),
        frames.mean(),
        frames.std(),
        totals.mean(),
        totals.std(),
        f"{device},{use_coroutines}",
    )


def benchmark_file(filename, n, use_coroutines, which="both"):
    # CPU
    r = []
    if which == "both" or which == "cpu":
        r.append(benchmark_file_single(filename, n, "cpu", use_coroutines))

    # GPU
    if which == "both" or which == "cuda":
        r.append(benchmark_file_single(filename, n, "cuda", use_coroutines))

    return r


def save_result(results_file, row, filename):
    with open(results_file, "a") as results:
        results.write(
            f"{filename},{','.join([(x if isinstance(x, str) else str(x.item())) for x in row])}\n"
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("device", type=str, choices=["cpu", "cuda", "both"])
    parser.add_argument(
        "--no_coroutines", default=False, action=argparse.BooleanOptionalAction
    )
    args = parser.parse_args()
    n = 64
    results_file = "gpu_results.csv"
    # files = ["aestream/example/davis.aedat4", "cam0197.aedat4", "cam0213.aedat4"]
    files = ["cam0197.aedat4"]
    for file in files:
        print(f"Benchmarking {file}")
        rows = benchmark_file(file, n, not args.no_coroutines, which=args.device)
        for row in rows:
            save_result(results_file, row, file)
