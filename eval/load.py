import glob
import os

import pandas as pd

def get_positions(path, seed):
    print("Getting positions")
    position_lines = []
    positions = pd.DataFrame()
    
    with open(path, 'r') as log:
        for lnumber, line in enumerate(log):
            if "Position:" not in line:
                continue
                
            position_lines.append(lnumber)

            _, x, y, addr = line.split(' ')
            x_val = x.split('=')[1].strip(',')
            y_val = y.split('=')[1].strip(',')
            addr_val = addr.split('=')[1].strip()

            tmp_df = {
                "X": x_val,
                "Y": y_val,
                "Address": addr_val,
                "Seed": seed
            }

            positions = positions.append(tmp_df, ignore_index=True)
                
    return positions, position_lines

def load_file(path):
    print(f"Loading {path}")
    
    basename = os.path.basename(path)
    filename = os.path.splitext(basename)[0]
    
    print("Setting config params")
    nodes, area, freq, bps, sps, bw, payload, msg_per_node, seed = filename.split('_')
    
    positions, position_lines = get_positions(path, seed)
    
    df = pd.read_csv(path, skiprows=position_lines)
    
    df["Nodes"]         = positions["Nodes"]         = int(nodes)
    df["Area"]          = positions["Area"]          = int(area)
    df["Frequency"]     = positions["Frequency"]     = int(freq)
    df["Bits/s"]        = positions["Bits/s"]        = int(bps)
    df["Symbols/s"]     = positions["Symbols/s"]     = int(sps)
    df["Bandwidth"]     = positions["Symbols/s"]     = int(bw)
    df["Payload"]       = positions["Payload"]       = int(payload)
    df["Messages/Node"] = positions["Messages/Node"] = int(msg_per_node)
    df["Seed"]          = positions["Seed"]          = int(seed)
    
    df.loc[(df["Symbols/s"] == 61) & (df["Bits/s"] == 292) & (df["Payload"] == 50), "Mode"] = "SF12/125kHz/50B"
    df.loc[(df["Symbols/s"] == 977) & (df["Bits/s"] == 5468) & (df["Payload"] == 240), "Mode"] = "SF7/125kHz/240B"
    df.loc[(df["Symbols/s"] == 1954) & (df["Bits/s"] == 10937) & (df["Payload"] == 240), "Mode"] = "SF7/250kHz/240B"
    
    return df, positions
    
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

    _dfs = [load_file(p) for p in all_files]
    
    dfs = [df[0] for df in _dfs]
    pos = [df[1] for df in _dfs]
    
    print("Building giganonormous df")
    df = pd.concat(dfs)
    positions = pd.concat(pos)
    
    print("Pickling")
    df.to_pickle(f"{path}df.gz")
    positions.to_pickle(f"{path}pos.gz")
    
    return df, positions
    

if __name__ == '__main__':
    load_data("../results/2022-01-04_143612/")