const isDev = () => {
    if (process.env.DEV_BUILD === undefined || process.env.DEV_BUILD === null)
        return false;
    return process.env.DEV_BUILD === "1" || process.env.DEV_BUILD.toLowerCase() === "true"
}

export default isDev;