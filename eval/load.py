import glob
import os
import sys
from datetime import datetime

import pandas as pd
import numpy as np

def get_positions(path):
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
    
    print(f"{datetime.now()} - Getting positions", file=log_file, flush=True)
    positions, position_lines = get_positions(path)
    
    print(f"{datetime.now()} - Reading CSV", file=log_file, flush=True)
    df = pd.read_csv(path, skiprows=position_lines)
    
    print(f"{datetime.now()} - Getting sender IDs", file=log_file, flush=True)
    sender_id_dict = {}
    for index, row in df[df["Event"] == "TX"].iterrows():
        sender_id_dict[row["Packet ID"]] = row["Sender ID"]
    
    df.loc[df["Event"] != "TX", "Sender ID"] = df[df["Event"] != "TX"]["Packet ID"].apply(lambda x: sender_id_dict[x])
    
    print(f"{datetime.now()} - Setting config params", file=log_file, flush=True)
    nodes, area, _, sf, _, bw, payload, msg_per_node, seed = filename.split('_')
    df["Nodes"]            = int(nodes)
    df["Area"]             = int(area)
    df["Spreading Factor"] = int(sf)
    df["Bandwidth"]        = int(bw)
    df["Payload"]          = int(payload)
    df["Messages/Node"]    = int(msg_per_node)
    df["Seed"]             = int(seed)
    
    print(f"{datetime.now()} - Converting simulation time", file=log_file, flush=True)
    df['Simulation Time'] = pd.to_timedelta(df["Simulation Time"].map(lambda x: float(x.lstrip('+').rstrip('ns'))), unit="ns")
    
    print(f"{datetime.now()} - Setting modes", file=log_file, flush=True)
    df.loc[(df["Bandwidth"] == 125000) & (df["Spreading Factor"] == 12) & (df["Payload"] == 51),  "Mode"]  = "SF12, 125kHz, 51B"
    df.loc[(df["Bandwidth"] == 125000) & (df["Spreading Factor"] == 9)  & (df["Payload"] == 51),  "Mode"]  = "SF9, 125kHz, 51B"
    df.loc[(df["Bandwidth"] == 125000) & (df["Spreading Factor"] == 9)  & (df["Payload"] == 115),  "Mode"] = "SF9, 125kHz, 115B"
    df.loc[(df["Bandwidth"] == 125000) & (df["Spreading Factor"] == 7)  & (df["Payload"] == 51), "Mode"]   = "SF7, 125kHz, 51B"
    df.loc[(df["Bandwidth"] == 125000) & (df["Spreading Factor"] == 7)  & (df["Payload"] == 222),  "Mode"] = "SF7, 125kHz, 222B"
    df.loc[(df["Bandwidth"] == 250000) & (df["Spreading Factor"] == 7)  & (df["Payload"] == 222),  "Mode"] = "SF7, 250kHz, 222B"
        
    print(f"{datetime.now()} - Setting position parameters", file=log_file, flush=True)        
    df["Sender Position X"] = df["Sender ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[0])
    df["Sender Position Y"] = df["Sender ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[1])
    df["Receiver Position X"] = df["Receiver ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[0])
    df["Receiver Position Y"] = df["Receiver ID"].apply(lambda x: positions.get(x, (np.NaN, np.NaN))[1])
    
    print(f"{datetime.now()} - Processing distances", file=log_file, flush=True)
    df['Distance'] = get_distance(df["Sender Position X"], df["Sender Position Y"], df["Receiver Position X"], df["Receiver Position Y"])
        
    return df
    
def load_data(path, param_filter={}):
    nodes = area = sf = bw = payload = msg_per_node = None
    
    if 'nodes' in param_filter:
        nodes = param_filter['nodes']
    else:
        nodes = "*"
    if 'area' in param_filter:
        area = param_filter['area']
    else:
        area = "*"
    if 'sf' in param_filter:
        sf = param_filter['sf']
    else:
        sf = "*"
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
        
    filter_str = f"{nodes}_{area}_*_{sf}_*_{bw}_{payload}_{msg_per_node}_*"
          
    all_files = glob.glob(f"{path}{filter_str}.log")
    
    ok_runs = ("35039.log", "35040.log", "35041.log", "35042.log", "35043.log")

    dfs = [load_file(p, log_file) for p in all_files if p.endswith(ok_runs)]
    
    print(f"{datetime.now()} - Building gigantonormous df", file=log_file, flush=True)
    df = pd.concat(dfs)
    
    # For some plot we want to plot many data points. Thus, we have to use just a sample because the data and the figure get unmanageble
    sampled_df = df.sample(frac=.05)
    sampled_df = sampled_df[~sampled_df["Messages/Node"].isin([1, 20, 30, 40, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200])]
    
    print(f"{datetime.now()} - Done", file=log_file, flush=True)
    print("Done")
    log_file.close()
    
    return df, sampled_df
    

if __name__ == '__main__':
    load_data("../results/2022-01-04_143612/")