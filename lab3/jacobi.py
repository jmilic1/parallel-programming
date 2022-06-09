def jacobistep(psinew, psi, m, n):
    # calculate new psi
    for i in range(1, m + 1):
        for j in range(1, n + 1):
            psinew[i * (m + 2) + j] = 0.25 * (
                    psi[(i - 1) * (m + 2) + j] + psi[(i + 1) * (m + 2) + j] + psi[i * (m + 2) + j - 1] + psi[
                i * (m + 2) + j + 1])
    return psinew


def deltasq(newarr, oldarr, m, n):
    # calculate the error
    error = 0.0
    for i in range(1, m + 1):
        for j in range(1, n + 1):
            temp = newarr[i * (m + 2) + j] - oldarr[i * (m + 2) + j]
            error += temp * temp
    return error
