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

	import { useNavigate } from "svelte-navigator";
	const navigate = useNavigate();
</script>

<Card class="login-card">
	<CardHeader>
		<CardTitle>Login</CardTitle>
	</CardHeader>
	<CardBody>
		<FormGroup>
			<Label for="email">Email</Label>
			<Input
				type="email"
				name="email"
				id="email"
				placeholder="example@email.org"
			/>
			<Label for="password">Password</Label>
			<Input
				type="password"
				name="password"
				id="password"
				placeholder="something long and complex"
			/>
		</FormGroup>
		<ButtonToolbar class="float-right login-button-toolbar">
			<Button
				on:click={() => {
					navigate("/");
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
