import glob
import os
import sys
from datetime import datetime

import pandas as pd
import numpy as np

def get_positions(path, seed):
    position_lines = []
    positions = {}
    
    with open(path, 'r') as log:
        for lnumber, line in enumerate(log):
            if "Position:" not in line:
                continue
                
            position_lines.append(lnumber)

            _, x, y, addr = line.split(' ')
            x_val = x.split('=')[1].strip(',')
            y_val = y.split('=')[1].strip(',')
            addr_val = addr.split('=')[1].strip()
            
            positions[float(addr_val)] = (int(x_val), int(y_val))
          
    return positions, position_lines

def get_distance(xpos_sender, ypos_sender, xpos_recv, ypos_recv):
    x_delta = xpos_sender - xpos_recv
    y_delta = ypos_sender - ypos_recv
    
    return np.sqrt(x_delta**2 + y_delta**2)

def load_file(path, log_file):
    basename = os.path.basename(path)
    filename = os.path.splitext(basename)[0]
    print(f"{datetime.now()} - Loading {filename}", file=log_file, flush=True)
    nodes, area, freq, bps, sps, bw, payload, msg_per_node, seed = filename.split('_')
    
    print(f"{datetime.now()} - Getting positions", file=log_file, flush=True)
    positions, position_lines = get_positions(path, seed)
    
    print(f"{datetime.now()} - Reading CSV", file=log_file, flush=True)
    df = pd.read_csv(path, skiprows=position_lines)
    
    print(f"{datetime.now()} - Getting sender IDs", file=log_file, flush=True)
    sender_id_dict = {}
    for index, row in df[df["Event"] == "TX"].iterrows():
        sender_id_dict[row["Packet ID"]] = row["Sender ID"]
    
    df.loc[df["Event"] == "RX", "Sender ID"] = df[df["Event"] == "RX"]["Packet ID"].apply(lambda x: sender_id_dict[x])
    
    print(f"{datetime.now()} - Setting config params", file=log_file, flush=True)
    df["Nodes"]         = int(nodes)
    df["Area"]          = int(area)
    df["Frequency"]     = int(freq)
    df["Bits/s"]        = int(bps)
    df["Symbols/s"]     = int(sps)
    df["Bandwidth"]     = int(bw)
    df["Payload"]       = int(payload)
    df["Messages/Node"] = int(msg_per_node)
    df["Seed"]          = int(seed)
    
    print(f"{datetime.now()} - Converting simulation time", file=log_file, flush=True)
    df['Simulation Time'] = pd.to_timedelta(df["Simulation Time"].map(lambda x: float(x.lstrip('+').rstrip('ns'))), unit="ns")
    
    print(f"{datetime.now()} - Setting modes", file=log_file, flush=True)
    df.loc[(df["Bandwidth"] == 125) & (df["Symbols/s"] == 61)   & (df["Bits/s"] == 292)   & (df["Payload"] == 50),  "Mode"] = "SF12, 125kHz, 292B/s,   50B"
    df.loc[(df["Bandwidth"] == 125) & (df["Symbols/s"] == 977)  & (df["Bits/s"] == 5468)  & (df["Payload"] == 50),  "Mode"] = "SF7,  125kHz, 5468B/s,  50B"
    df.loc[(df["Bandwidth"] == 125) & (df["Symbols/s"] == 977)  & (df["Bits/s"] == 5468)  & (df["Payload"] == 240), "Mode"] = "SF7,  125kHz, 5468B/s,  240B"
    df.loc[(df["Bandwidth"] == 250) & (df["Symbols/s"] == 1954) & (df["Bits/s"] == 10937) & (df["Payload"] == 50),  "Mode"] = "SF7,  250kHz, 10937B/s, 50B"
    df.loc[(df["Bandwidth"] == 250) & (df["Symbols/s"] == 1954) & (df["Bits/s"] == 10937) & (df["Payload"] == 240), "Mode"] = "SF7,  250kHz, 10937B/s, 240B"
    
    print(f"{datetime.now()} - Setting position parameters", file=log_file, flush=True)        
    df["Sender Position X"] = df["Sender ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[0])
    df["Sender Position Y"] = df["Sender ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[1])
    df["Receiver Position X"] = df["Receiver ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[0])
    df["Receiver Position Y"] = df["Receiver ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[1])
    
    print(f"{datetime.now()} - Processing distances", file=log_file, flush=True)
    df['Distance'] = get_distance(df["Sender Position X"], df["Sender Position Y"], df["Receiver Position X"], df["Receiver Position Y"])
    df['Distance Labels'] = pd.cut(
        x=df['Distance'],
        bins=[
            0,
            500,
            1000,
            1500,
            2000,
            2500,
            3000,
            3500,
            4000,
            4500,
            5000,
            10000
        ],
        labels=[
            "<500",
            "500-1000",
            "1000-1500",
            "1500-2000",
            "2000-2500",
            "2500-3000",
            "3000-3500",
            "3500-4000",
            "4000-4500",
            "4500-5000",
            ">5000"
        ]
    )
        
    return df
    
def load_data(path, param_filter={}):
    nodes = area = freq = bps = sps = bw = payload = msg_per_node = None
    
    if 'nodes' in param_filter:
        nodes = param_filter['nodes']
    else:
        nodes = "*"
    if 'area' in param_filter:
        area = param_filter['area']
    else:
        area = "*"
    if 'freq' in param_filter:
        freq = param_filter['freq']
    else:
        freq = "*"
    if 'bps' in param_filter:
        bps = param_filter['bps']
    else:
        bps = "*"
    if 'sps' in param_filter:
        sps = param_filter['sps']
    else:
        sps = "*"
    if 'bw' in param_filter:
        bw = param_filter['bw']
    else:
        bw = "*"
    if 'payload' in param_filter:
        payload = param_filter['payload']
    else:
        payload = "*"
    if 'msg_per_node' in param_filter:
        msg_per_node = param_filter['msg_per_node']
    else:
        msg_per_node = "*"
        
    log_file = open("log.txt", "w")
        
    filter_str = f"{nodes}_{area}_{freq}_{bps}_{sps}_{bw}_{payload}_{msg_per_node}_*"
          
    all_files = glob.glob(f"{path}{filter_str}.log")

    dfs = [load_file(p, log_file) for p in all_files]
    
    print(f"{datetime.now()} - Building gigantonormous df", file=log_file, flush=True)
    df = pd.concat(dfs)
    
    print(f"{datetime.now()} - Pickling", file=log_file, flush=True)
    #df.to_pickle(f"{path}df.gz")
    
    print(f"{datetime.now()} - Done", file=log_file, flush=True)
    log_file.close()
    
    return df
    

if __name__ == '__main__':
    load_data("../results/2022-01-04_143612/")