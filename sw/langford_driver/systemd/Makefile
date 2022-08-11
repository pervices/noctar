all: install


install: 
	@echo -e 'Installing Langford systemd service agent.'
	@sudo cp langford.service /usr/lib/systemd/system/langford.service
	@echo -e '*****\n**To enable systemd service agent, run:'
	@echo -e '\t systemctl enable langford.service'
	@echo -e '**as root user. To immediately start the service:'
	@echo -e '\t systemctl start langford.service\n*****'

uninstall:
	@echo -e 'Attempting to disable systemd service.'
	@-systemctl disable langford.service
	@echo -e 'Removing systemd service agent.'
	@-sudo rm /usr/lib/systemd/system/langford.service
