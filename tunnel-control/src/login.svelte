<script>
	import {
		Card,
		CardBody,
		CardHeader,
		CardTitle,
		FormGroup,
		Input,
		Label,
		Button,
		ButtonToolbar,
	} from "sveltestrap";
	import { sha512 } from "./utility/hash";
	import { useNavigate } from "svelte-navigator";
	import { setSession } from "./session";

	const navigate = useNavigate();

	let email;
	let password;

	const login = async () => {
		const hashedPw = await sha512(password);
		const basic = btoa(`${email}:${hashedPw}`);
		fetch("/api/auth", {
			method: "GET",
			headers: {
				Authorization: `Basic ${basic}`,
			},
		}).then((res) => {
			console.log(res.status);
			if (res.status === 200) {
				res.text().then((token) => {
					email = "";
					password = "";
					setSession(token);
					navigate("/");
				});
			} else {
				alert("Invalid credentials");
			}
		});
	};
</script>

<Card class="login-card">
	<CardHeader>
		<CardTitle>Login</CardTitle>
	</CardHeader>
	<CardBody>
		<FormGroup>
			<Label for="email">Email / Username</Label>
			<Input
				type="email"
				name="email"
				id="email"
				placeholder="example@email.org"
				bind:value={email}
			/>
			<Label for="password">Password</Label>
			<Input
				type="password"
				name="password"
				id="password"
				placeholder="something long and complex"
				bind:value={password}
			/>
		</FormGroup>
		<ButtonToolbar class="float-right login-button-toolbar">
			<Button
				on:click={() => {
					login();
				}}>Login</Button
			>
			<Button
				on:click={() => {
					navigate("/");
				}}>Abort</Button
			>
		</ButtonToolbar>
	</CardBody>
</Card>

<style>
	:global(.login-card) {
		left: 25%;
		top: 25%;
		right: 25%;
		position: absolute;
	}

	:global(.login-button-toolbar) {
		justify-content: flex-end;
		margin-left: auto;
	}
</style>
