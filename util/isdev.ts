const isDev = () => {
    return process.env.DEV_BUILD === "1" || process.env.DEV_BUILD.toLowerCase() === "true"
}

export default isDev;