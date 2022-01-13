import glob
import os

import pandas as pd
import numpy as np

def get_positions(path, seed):
    print("Getting positions")
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
            
            positions[addr_val] = (int(x_val), int(y_val))
          
    return positions, position_lines

def load_file(path):
    print(f"Loading {path}")
    
    basename = os.path.basename(path)
    filename = os.path.splitext(basename)[0]
    
    print("Setting config params")
    nodes, area, freq, bps, sps, bw, payload, msg_per_node, seed = filename.split('_')
    
    positions, position_lines = get_positions(path, seed)
    
    df = pd.read_csv(path, skiprows=position_lines)
    
    df["Nodes"]         = int(nodes)
    df["Area"]          = int(area)
    df["Frequency"]     = int(freq)
    df["Bits/s"]        = int(bps)
    df["Symbols/s"]     = int(sps)
    df["Bandwidth"]     = int(bw)
    df["Payload"]       = int(payload)
    df["Messages/Node"] = int(msg_per_node)
    df["Seed"]          = int(seed)
    
    df.loc[(df["Symbols/s"] == 61) & (df["Bits/s"] == 292) & (df["Payload"] == 50), "Mode"] = "SF12/125kHz/50B"
    df.loc[(df["Symbols/s"] == 977) & (df["Bits/s"] == 5468) & (df["Payload"] == 240), "Mode"] = "SF7/125kHz/240B"
    df.loc[(df["Symbols/s"] == 1954) & (df["Bits/s"] == 10937) & (df["Payload"] == 240), "Mode"] = "SF7/250kHz/240B"
    
    x_pos_sender = []
    y_pos_sender = []
    x_pos_receiver = []
    y_pos_receiver = []
    for _, data in df.iterrows():
        xpos, ypos = positions[data["Sender Address"]]
        x_pos_sender.append(xpos)
        y_pos_sender.append(ypos)
        xpos, ypos = positions.get(data["Receiver Address"], (np.NaN, np.NaN))
        x_pos_receiver.append(xpos)
        y_pos_receiver.append(ypos)
        
    df["Sender Position X"] = x_pos_sender
    df["Sender Position Y"] = y_pos_sender
    df["Receiver Position X"] = x_pos_receiver
    df["Receiver Position Y"] = y_pos_receiver
        
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
        
    filter_str = f"{nodes}_{area}_{freq}_{bps}_{sps}_{bw}_{payload}_{msg_per_node}_*"
          
    all_files = glob.glob(f"{path}{filter_str}.log")

    dfs = [load_file(p) for p in all_files]
    
    print("Building giganonormous df")
    df = pd.concat(dfs)
    
    #print("Pickling")
    #df.to_pickle(f"{path}df.gz")
    
    print("Done")
    
    return df
    

if __name__ == '__main__':
    load_data("../results/2022-01-04_143612/")