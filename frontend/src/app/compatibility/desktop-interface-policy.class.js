(function(osimis) {

    function DesktopInterfacePolicy() {
        var _this = this;
        
        // Create observable. Not used in this policy.
        this.onUpdate = new osimis.Listener();

                // Manage portrait / landscape orientations
        // 1. Create a portrait orientation media query. See 
        // http://caniuse.com/#search=matchMedia for browser compatibility.
        var mediaQuery = window.matchMedia("(orientation: portrait)");
        var orientation = mediaQuery.matches ? 'portrait' : 'landscape';
        // 2. Register current state
        this._onOrientationChanged(mediaQuery);
        // 3. Listen to changes
        mediaQuery.addListener(function(mediaQuery) {
            var orientation = mediaQuery.matches ? 'portrait' : 'landscape';
            _this._onOrientationChanged(orientation);
        });
        // 4. Unlisten to changes on destroy
        // @todo Unlisten to changes on destroy

        // Set initial UI values
        // 1. Always enable top side
        this.enableLayoutTop = true;
        this.enableLayoutTopLeft = true;
        this.enableLayoutTopRight = true;
        this.enableToolbar = true;
        // 2. Enable left/right side on landsape orientation
        this.enableLayoutLeft = (orientation === 'landscape');
        this.enableLayoutLeftBottom = (orientation === 'landscape');
        this.enableLayoutRight = (orientation === 'landscape');
        // 3. Display aside handles on landscape mobile (buttons toggling the aside)
        this.enableLayoutLeftHandles = true;
        this.enableLayoutRightHandles = true;
        // 4. Show notice (bottom series selector) on portrait orientation with 
        this.enableNotice = (orientation === 'portrait');
        this.noticeText = 'Change your mobile orientation to display series list.';
    }

    DesktopInterfacePolicy.prototype.onUpdate = _.noop;

    DesktopInterfacePolicy.prototype._onOrientationChanged = function(orientation) {
        // On portrait orientation
        if (orientation === 'portrait') {
            // Disable left layout
            this.enableLayoutLeft = false;
            this.enableLayoutLeftBottom = false;
            this.enableLayoutRight = false;
            
            // Show text notice
            this.enableNotice = true;
        }

        // On landscape orientation
        if (orientation === 'landscape') {
            // Enable left/right layout
            this.enableLayoutLeft = true;
            this.enableLayoutLeftBottom = true;
            this.enableLayoutRight = true;

            // Hide text notice
            this.enableNotice = false;
        }

        // Trigger changes, so AngularJS directives can apply the interface
        // updates to their viewmodel and trigger a $digest cycle. This is only 
        // used by the `wvWebviewer` directive.
        this.onUpdate.trigger();
    }

    osimis.DesktopInterfacePolicy = DesktopInterfacePolicy;

})(this.osimis || (this.osimis = {}));