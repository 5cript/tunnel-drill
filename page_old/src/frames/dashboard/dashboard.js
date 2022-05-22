import './dashboard.css'
import { LineChart, Line, CartesianGrid, XAxis, YAxis } from 'recharts';
import { publishersState } from '../../state/publishers';
import { useRecoilState } from 'recoil';
import Backend from '../../backend_api/backend';
import { useEffect } from 'react';

const data = [{name: 'Page A', uv: 400, pv: 2400, amt: 2400}];

const Dashboard = ({
    backend
}) => {
    const [publishers, setPublishers] = useRecoilState(publishersState);

    // useEffect(() => {
    //     setInterval(() => {
    //         backend.publishers.list().then((publishers) => {
    //             setPublishers(publishers);
    //         });
    //     }, 5000);
    // }, []);

    console.log(publishers);

    return <>
        <LineChart width={400} height={400} data={data}>
            <Line type="monotone" dataKey="uv" stroke="#8884d8" />
            <CartesianGrid stroke="#ccc" />
            <XAxis dataKey="name" />
            <YAxis />
        </LineChart>
        <div>
            {JSON.stringify(publishers)}
        </div>
    </>
}

export default Dashboard