<script>
	import {
		Navbar,
		Button,
		Dropdown,
		DropdownToggle,
		DropdownMenu,
		DropdownItem,
	} from "sveltestrap";
	import { createEventDispatcher } from "svelte";
	import { Link } from "svelte-navigator";

	const dispatch = createEventDispatcher();
	let class_;
	export { class_ as class };
	let user = {
		loggedIn: false,
	};
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
			<DropdownToggle nav caret>User Control</DropdownToggle>
			<DropdownMenu end>
				{#if user.loggedIn}
					<DropdownItem>Profile</DropdownItem>
					<DropdownItem>Publishers</DropdownItem>
					<DropdownItem divider />
					<DropdownItem>Sign Out</DropdownItem>
				{:else}
					<DropdownItem>
						<Link to="/login">Sign In</Link>
					</DropdownItem>
				{/if}
			</DropdownMenu>
		</Dropdown>
	</Navbar>
</div>

<style>
</style>
