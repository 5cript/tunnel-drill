class RetryContext
{
    onRetry: () => void;
    currentTime: number;
    maxTime: number;
    timeout: any;
    
    constructor(onRetry: () => void, currentTime: number, maxTime?: number)
    {
        this.onRetry = onRetry;
        this.currentTime = currentTime;
        if (maxTime === undefined)
            maxTime = 60000;
        this.maxTime = maxTime;
    }

    private escalateTime = () => {
        this.currentTime = Math.min(this.maxTime, this.currentTime * 2);
    }
    private retryImpl = () => {
        this.timeout = setTimeout(() => {
            this.onRetry();
        }, this.currentTime);
    }
    retry = () => {
        this.escalateTime();
        this.retryImpl();
    }
    reset = (time?: number) => {
        if (time === undefined)
            time = 1000;
        this.currentTime = time;
    }
    abort = () => {
        clearTimeout(this.timeout);
    }
}

export default RetryContext