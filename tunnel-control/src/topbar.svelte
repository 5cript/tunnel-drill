<script>
	import {
		Navbar,
		Button,
		Dropdown,
		DropdownToggle,
		DropdownMenu,
		DropdownItem,
	} from "sveltestrap";
	import { createEventDispatcher, onDestroy } from "svelte";
	import { useNavigate } from "svelte-navigator";
	import { sessionTokenPayload } from "./stores.js";
	import { isLoggedIn, logout } from "./session.js";
	const navigate = useNavigate();

	const dispatch = createEventDispatcher();
	let class_;
	export { class_ as class };
	let user = $sessionTokenPayload.identity ?? "User Control";
	let loggedIn = isLoggedIn();

	const unsubSessionToken = sessionTokenPayload.subscribe((value) => {
		user = $sessionTokenPayload.identity ?? "User Control";
		loggedIn = isLoggedIn();
	});
	onDestroy(unsubSessionToken);
</script>

<div class={class_}>
	<Navbar class="py-0" expand="md">
		<Button
			dark
			on:click={() => {
				dispatch("toggleMenu");
			}}>Menu</Button
		>
		<h1 class="text-light">Tunnel Control</h1>
		<Dropdown>
			<DropdownToggle nav caret>{user}</DropdownToggle>
			<DropdownMenu end>
				{#if loggedIn}
					<DropdownItem>Profile</DropdownItem>
					<DropdownItem>Publishers</DropdownItem>
					<DropdownItem divider />
					<DropdownItem
						on:click={() => {
							localStorage.removeItem("sessionToken");
							logout();
						}}>Sign Out</DropdownItem
					>
				{:else}
					<DropdownItem
						on:click={() => {
							navigate("/login");
						}}
					>
						Sign In
					</DropdownItem>
				{/if}
			</DropdownMenu>
		</Dropdown>
	</Navbar>
</div>

<style>
</style>
