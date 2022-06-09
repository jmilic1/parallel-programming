def boundarypsi(psi, m, b, h, w):
    for i in range(b + 1, b + w):
        psi[i * (m + 2) + 0] = i - b
    for i in range(b + w, m + 1):
        psi[i * (m + 2) + 0] = w

    for j in range(1, h + 1):
        psi[(m + 1) * (m + 2) + j] = w
    for j in range(h + 1, h + w):
        psi[(m + 1) * (m + 2) + j] = w - j + h
    return psi


def boundaryzet(zet, psi, m, n):
    for i in range(1, m + 1):
        zet[i * (m + 2) + 0] = 2.0 * (psi[i * (m + 2) + 1] - psi[i * (m + 2) + 0]);
        zet[i * (m + 2) + n + 1] = 2.0 * (psi[i * (m + 2) + n] - psi[i * (m + 2) + n + 1])
    for j in range(1, n):
        zet[0 * (m + 2) + j] = 2.0 * (psi[1 * (m + 2) + j] - psi[0 * (m + 2) + j])
    for j in range(1, n):
        zet[(m+1)*(m+2)+j] = 2.0*(psi[m*(m+2)+j]-psi[(m+1)*(m+2)+j]);
    return zet
