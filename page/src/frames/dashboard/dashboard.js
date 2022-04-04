import './dashboard.css'
import { LineChart, Line, CartesianGrid, XAxis, YAxis } from 'recharts';
import { publishersState } from '../../state/publishers';
import { useRecoilState } from 'recoil';

const data = [{name: 'Page A', uv: 400, pv: 2400, amt: 2400}];

const Dashboard = () => {
    const [publishers, setPublishers] = useRecoilState(publishersState);

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