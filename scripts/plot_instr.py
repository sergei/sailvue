import argparse

import matplotlib.pyplot as plt
import numpy as np


def proces_logs(args):

    # Speed data
    process_sources(['speed-DST810', 'speed-H5000-BS', 'speed-H5000-CPU'],
                    ['sow', 'sow-sog'],
                    args.work_dir)

    # Wind data
    process_sources(['wind-H5000-MHU', 'wind-WS310', 'wind-H5000-CPU'],
                    ['awa', 'aws', 'twa', 'tws'],
                    args.work_dir)

    # Heading data
    process_sources(['heading-Precision-9', 'heading-ZG100'],
                    ['mag', 'mag-cog'],
                    args.work_dir)


def process_sources(sources, values, work_dir):
    vals = read_values(sources, values, work_dir)
    plot_values(sources, values, vals)


def plot_values(sources, values, vals):
    for value in values:
        plt.figure()
        for source in sources:
            label = source
            val = np.array(vals[source][value])
            if np.isnan(val).all():
                continue
            if '-' in value:
                mean = np.nanmean(val)
                std = np.nanstd(val)
                label += f' {mean:.2f} +/- {std:.2f}'
            plt.plot(val, label=label)
        plt.legend()
        plt.title(value)
        plt.grid(True)


def read_values(sources, values, work_dir):
    vals = {}
    for source in sources:

        vals[source] = {}
        for value in values:
            vals[source][value] = []

        csv_file_name = work_dir + '/' + source + '-instr.csv'
        with open(csv_file_name, 'r') as f:
            print(f'Reading {csv_file_name}')
            for line in f:
                t = line.split(',')
                for value in values:
                    if '-' in value:
                        exp = value.split('-')
                        op1 = exp[0]
                        op2 = exp[1]
                    else:
                        op1 = None
                        op2 = None
                    for i in range(len(t)-1):
                        v = t[i + 1]
                        if t[i] == value:
                            val = float(v) if len(v) > 0 else np.nan
                            vals[source][value].append(val)
                        if op1 is not None and t[i] == op1:
                            val1 = float(v) if len(v) > 0 else np.nan
                        if op2 is not None and t[i] == op2:
                            val2 = float(v) if len(v) > 0 else np.nan
                    if op1 is not None and op2 is not None:
                        diff = val1 - val2
                        if diff > 180:
                            diff -= 360
                        elif diff < -180:
                            diff += 360
                        vals[source][value].append(diff)

            print(f'Processed {len(vals[source][values[0]])} values')
    return vals


if __name__ == '__main__':

    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
    parser.add_argument("--work-dir", help="Working directory", default='/private/tmp/ydn-csv')

    proces_logs(parser.parse_args())

    plt.show()
