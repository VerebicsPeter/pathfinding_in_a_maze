import sys
import random
import numpy as np
import matplotlib.pyplot as plt

M_EMPT = 0
M_WALL = 1


def init_maze(size):
    L = size
    lattice = [[M_WALL for _ in range(L)] for _ in range(L)]
    visited = [[False  for _ in range(L)] for _ in range(L)]
    
    idxs = range(1, L, 2)
    for i in idxs:
        for j in idxs:
            lattice[i][j] = M_EMPT 
    return lattice, visited


def make_maze_depthfs(size, start_idx=None):
    if not start_idx: start_idx = (1,1)
    sx, sy = start_idx
    lattice, visited = init_maze(size)
    assert lattice[sx][sy] == M_EMPT
    L = size
    
    offs = [(-1, 0), (+1,0), (0, -1), (0, +1)]
    stack = [start_idx]
    while stack:
        cx, cy = stack[-1]
        visited[cx][cy] = True
        
        directions = offs[:]
        random.shuffle(directions)
        
        removed = False
        for dx, dy in directions:
            nx, ny = cx + 2*dx, cy + 2*dy
            if not (0 <= nx < L and 0 <= ny < L):
                continue
            if not visited[nx][ny]:
                # remove wall
                wx, wy = cx + dx, cy + dy
                lattice[wx][wy] = M_EMPT 
                removed = True
                # push neighbor
                stack.append((nx, ny))
                break
        
        if not removed: stack.pop()  # backtrack

    return lattice


def make_maze_kruskal(size, start_idx=None):
    sx, sy = start_idx
    lattice, _ = init_maze(size)
    assert lattice[sx][sy] == M_EMPT
    L = size
    Eidxs = range(1, L, 2)

    # Initialize disjoint sets (each empty cell is its own set)
    parent = {}
    for i in Eidxs:
        for j in Eidxs:
            lattice[i][j] = M_EMPT
            parent[(i, j)] = (i, j)

    def find(x):
        # Path compression
        if parent[x] != x:
            parent[x] = find(parent[x])
        return parent[x]

    def union(a, b):
        pa, pb = find(a), find(b)
        if pa != pb:
            parent[pa] = pb
            return True
        return False

    # Build list of walls that could connect two cells
    walls = []
    for i in Eidxs:
        for j in Eidxs:
            if i < L - 2:  # wall between (i,j) and (i+2,j)
                walls.append(((i + 1, j), (i, j), (i + 2, j)))
            if j < L - 2:  # wall between (i,j) and (i,j+2)
                walls.append(((i, j + 1), (i, j), (i, j + 2)))

    random.shuffle(walls)

    # Process walls
    for wall, a, b in walls:
        if union(a, b):
            wx, wy = wall
            lattice[wx][wy] = M_EMPT

    return lattice


if __name__ == "__main__":
    if len(sys.argv) != 3:
        exit(1)
    else:
        ALGOS = {
            "depthfs": make_maze_depthfs,
            "kruskal": make_maze_kruskal,
        }
        
        
        SIZE = int(sys.argv[1])
        ALGO = str(sys.argv[2])
        assert ALGO in ALGOS, f"No algorithm named {ALGO}"
        
        func = ALGOS[ALGO]
        
        maze = np.array(func(SIZE, (1,1)), dtype=np.uint8)
        
        print(maze)
        
        maze.tofile("maze.bin")
