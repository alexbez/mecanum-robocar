"""Render FreeCAD shapes to PNG with matplotlib (orthographic, flat shading).

All triangles of all shapes go into one collection, so matplotlib's
depth sorting works across parts (good enough for previews; it is a
painter's algorithm, not a z-buffer).
"""
import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection


def _subdivide(T, max_edge):
    """Split triangles 1 -> 4 until no edge is longer than max_edge.

    Flat faces tessellate into long slivers; small triangles make the
    painter's-algorithm depth sort much more reliable.
    """
    out = []
    while len(T):
        e = np.max(np.stack([np.linalg.norm(T[:, 0] - T[:, 1], axis=1),
                             np.linalg.norm(T[:, 1] - T[:, 2], axis=1),
                             np.linalg.norm(T[:, 2] - T[:, 0], axis=1)]), axis=0)
        small = e <= max_edge
        out.append(T[small])
        B = T[~small]
        if not len(B):
            break
        a, b, c = B[:, 0], B[:, 1], B[:, 2]
        ab, bc, ca = (a + b) / 2, (b + c) / 2, (c + a) / 2
        T = np.concatenate([np.stack([a, ab, ca], 1), np.stack([ab, b, bc], 1),
                            np.stack([ca, bc, c], 1), np.stack([ab, bc, ca], 1)])
    return np.concatenate(out) if out else np.zeros((0, 3, 3))


def _tris(shape, tol=0.15, max_edge=4.0):
    pts, tri = shape.tessellate(tol)
    if not tri:
        return np.zeros((0, 3, 3))
    P = np.array([[p.x, p.y, p.z] for p in pts])
    return _subdivide(P[np.array(tri)], max_edge)


def render(items, out, views=((30, -60),), titles=None, size=5, axes=False):
    """items: list of (shape, (r, g, b)). views: list of (elev, azim) in degrees."""
    tris, cols = [], []
    for s, c in items:
        T = _tris(s)
        if len(T):
            tris.append(T)
            cols.append(np.repeat(np.array(c, float)[None, :], len(T), 0))
    T = np.concatenate(tris)
    C = np.concatenate(cols)
    lo, hi = T.reshape(-1, 3).min(0), T.reshape(-1, 3).max(0)
    ctr, rad = (lo + hi) / 2, (hi - lo).max() / 2 * 0.62

    n = np.cross(T[:, 1] - T[:, 0], T[:, 2] - T[:, 0])
    n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-12

    fig = plt.figure(figsize=(size * len(views), size * 0.75))
    for i, (el, az) in enumerate(views):
        ax = fig.add_subplot(1, len(views), i + 1, projection="3d")
        ax.set_proj_type("ortho")
        e, a = np.radians(el), np.radians(az)
        cam = np.array([np.cos(e) * np.cos(a), np.cos(e) * np.sin(a), np.sin(e)])
        light = cam * 0.8 + np.array([0.2, 0.3, 0.5])
        light /= np.linalg.norm(light)
        shade = 0.30 + 0.70 * np.abs(n @ light)
        fc = np.clip(C * shade[:, None], 0, 1)
        coll = Poly3DCollection(T, facecolors=fc, edgecolors=fc, linewidths=0.05)
        ax.add_collection3d(coll)
        for k, f in enumerate((ax.set_xlim, ax.set_ylim, ax.set_zlim)):
            f(ctr[k] - rad, ctr[k] + rad)
        ax.set_box_aspect((1, 1, 1))
        ax.view_init(el, az)
        if not axes:
            ax.set_axis_off()
        if titles:
            ax.set_title(titles[i])
    plt.subplots_adjust(left=0, right=1, top=0.93, bottom=0, wspace=0)
    plt.savefig(out, dpi=120)
    plt.close(fig)
